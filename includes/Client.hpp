#pragma once

#include <string>

class Server;
class HttpRequest;
class HttpResponse;

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
  bool on_parse();

private:
  int fd;              // client fd
  int server_fd;       // このclientが接続しているserver fd
  HttpRequest request; // header情報, body, contentLengthなどの管理

  std::string request_buffer;  // recv用の受信buffer
  std::string response_buffer; // send用の送信buffer
  ssize_t response_sent;       // send済みのbytes数

  Client();
  Client(const Client &other);

  Client &operator=(const Client &other);
};
