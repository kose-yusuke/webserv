#pragma once

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include <string>

class VirtualHostRouter;

enum IOStatus {
  IO_CONTINUE,        // 現状維持（read/write継続）
  IO_READY_TO_WRITE,  // write監視をON
  IO_WRITE_COMPLETE,  // write監視をOFF
  IO_SHOULD_SHUTDOWN, // shutdown(fd, SHUT_WR)
  IO_SHOULD_CLOSE     // close(fd)
};

enum ClientState {
  CLIENT_ALIVE,       // 通常
  CLIENT_CGI_PENDING, // CGIをpollに噛ませるときに使う予定
  CLIENT_TIMED_OUT,   // Time-out; `time out`レスポンスを送りたい
  CLIENT_HALF_CLOSED, // SHUT_WR 済み; EOF or `Connection: close` 待ち
  // TIMED_OUTはレスポンスを送り次第, HALF_CLOSEDになる
  // CLOSEDなclientはdeleteするのでCLIENT_CLOSEDは省略
};

class Client {
public:
  Client(int clientfd, const VirtualHostRouter *router);
  ~Client();

  int get_fd() const;

  IOStatus on_read();
  IOStatus on_write();
  IOStatus on_timeout();

  bool is_timeout(time_t now) const;
  bool is_unresponsive(time_t now) const;

private:
  int fd_; // client fd
  ClientState client_state_;
  time_t timeout_sec_;
  time_t last_activity_;

  HttpResponse response_;    // responseの生成とqueue管理
  HttpRequest request_;      // header情報, body, contentLengthなどの管理
  CgiHandler   cgi_; 
  HttpRequestParser parser_; // header, bodyの解析管理
  ResponseEntry *current_entry_; // 現在送信中のresponse entry;
  size_t response_sent_;         // send済みのbytes数

  IOStatus on_parse();
  IOStatus on_half_close();
  bool has_response() const;

  void update_activity();

  Client(const Client &other);
  Client &operator=(const Client &other);
};
