#pragma once

#include <string>

class HttpRequest;
class HttpResponse;

enum IOStatus {
  IO_SUCCESS,  // 処理完了
  IO_CONTINUE, // 作業継続
  IO_FAILED    // エラーまたはクライアント切断
};

class Client {
public:
  Client(int clientFD);
  ~Client();

  IOStatus on_read();
  IOStatus on_write();
  bool on_parse();

private:
  int fd;
  HttpRequest request;         // header情報, body, contentLengthなどの管理
  std::string request_buffer;  // recv用の受信buffer
  std::string response_buffer; // send用の送信buffer
  ssize_t response_sent;        // send済みのbytes


  Client();
  Client(const Client &other);

  Client &operator=(const Client &other);
};
