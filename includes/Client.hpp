#pragma once

#include <string>

class HttpRequest;
class HttpResponse;

class Client {
public:
  int fd_;

  HttpRequest request_; // header, body, contentLengthなどの管理
  std::string requestBuffer_;  // recv or read の読み込みbuffer
  std::string responseBuffer_; // send or write するためのbuffer
  size_t responseSent_;

  Client(int clientFD);
  ~Client();

  bool on_read();
  bool on_parse();
  bool on_write();
  bool on_error();

private:
  Client();
  Client(const Client &other);

  Client &operator=(const Client &other);
};

/*
GET /index.html HTTP/1.1\r\n
Host: example.com\r\n
\r\n
GET /about.html HTTP/1.1\r\n
Host: example.com\r\n
\r\n
*/
