#pragma once

#include "CgiSession.hpp"
#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include <cstddef>

class VirtualHostRouter;

// Multiplexerにmonitorのon offを支持するのに使っているstatus
enum IOStatus {
  IO_CONTINUE,        // 現状維持（read/write継続）
  IO_READY_TO_WRITE,  // write監視をON
  IO_WRITE_COMPLETE,  // write監視をOFF
  IO_SHOULD_SHUTDOWN, // shutdown(fd, SHUT_WR)
  IO_SHOULD_CLOSE     // close(fd)
};

/*
HttpTransaction: リクエストの解析, CGI遷移, レスポンスの準備状態管理
 */
class HttpTransaction {
public:
  HttpTransaction(int clientfd, const VirtualHostRouter *router);
  ~HttpTransaction();

  void append_data(const char *raw, size_t length);
  void process_data();
  bool should_close();

  IOStatus decide_io_after_write(ConnectionPolicy connection_policy,
                                 ResponseType response_type);
  void reset_cgi_session();
  void handle_client_timeout();
  void handle_client_abort();

  bool has_response() const;
  ResponseEntry *get_response();
  void pop_response();

private:
  int fd_;
  HttpResponse response_;    // responseの生成とqueue管理
  HttpRequest request_;      // header情報, body, contentLengthなどの管理
  HttpRequestParser parser_; // header, bodyの解析管理
  CgiSession cgi_;           // cgi session の管理

  HttpTransaction(const HttpTransaction &other);
  HttpTransaction &operator=(const HttpTransaction &other);
};
