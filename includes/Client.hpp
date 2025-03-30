#pragma once

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include <string>

class Server;

enum IOStatus {
  IO_SUCCESS,  // 処理完了
  IO_CONTINUE, // 作業継続
  IO_CLOSED,   // client disconnectd (recv == 0)
  IO_ERROR     // I/O system call 失敗 (recv/send == -1)
};

class Client {
public:
  Client(int clientfd, int serverfd);
  ~Client();

  IOStatus on_read();
  IOStatus on_write();

private:
  int fd;                      // client fd
  int server_fd;               // このclientが接続しているserver fd
  HttpResponse response;       // responseの生成とqueue管理
  HttpRequest request;         // header情報, body, contentLengthなどの管理
  HttpRequestParser parser;    // header, bodyの解析管理
  std::string response_buffer; // 現在送信中のbuffer
  size_t response_sent;        // send済みのbytes数

  bool on_parse();
  bool has_response() const;

  Client(const Client &other);
  Client &operator=(const Client &other);
};
