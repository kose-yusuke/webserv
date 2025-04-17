#pragma once

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include <string>

class Server;

enum IOStatus {
  IO_CONTINUE,        // 現状維持（read/write継続）
  IO_READY_TO_WRITE,  // write監視をON
  IO_WRITE_COMPLETE,  // write監視をOFF
  IO_SHOULD_SHUTDOWN, // shutdown(fd, SHUT_WR)
  IO_SHOULD_CLOSE     // close(fd)
};

enum ClientState {
  CLIENT_ALIVE,      // 通常
  CLIENT_HALF_CLOSED // SHUT_WR 済み; EOF or `Connection: close` 待ち
};

class Client {
public:
  Client(int clientfd, int serverfd);
  ~Client();

  IOStatus on_read();
  IOStatus on_write();

private:
  int fd;                   // client fd
  int server_fd;            // このclientが接続しているserver fd
  HttpResponse response;    // responseの生成とqueue管理
  HttpRequest request;      // header情報, body, contentLengthなどの管理
  HttpRequestParser parser; // header, bodyの解析管理

  ClientState client_state;
  // std::string response_buffer; // 現在送信中のbuffer
  ResponseEntry *current_entry; // 現在送信中のresponse entry;
  size_t response_sent;         // send済みのbytes数

  IOStatus on_parse();
  IOStatus on_half_close();
  bool has_response() const;

  Client(const Client &other);
  Client &operator=(const Client &other);
};
