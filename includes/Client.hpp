#pragma once

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include <queue>
#include <string>

class Server;

enum IOStatus {
  IO_SUCCESS,  // 処理完了
  IO_CONTINUE, // 作業継続
  IO_FAILED    // エラーまたはクライアント切断
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
  HttpRequest request;      // header情報, body, contentLengthなどの管理
  HttpRequestParser parser; // header, bodyの解析管理
  std::queue<std::string> response_queue; // 複数のresponseを貯めるqueue
  size_t response_sent;                   // send済みのbytes数

  bool on_parse();

  Client(const Client &other);
  Client &operator=(const Client &other);
};
