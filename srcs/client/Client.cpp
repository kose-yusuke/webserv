
#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>

Client::Client(int clientfd, int serverfd)
    : fd(clientfd), server_fd(serverfd), response(),
      request(serverfd, response), parser(request), response_sent(0) {}

Client::~Client() {}

IOStatus Client::on_read() {
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes_read == 0) {
    logfd(LOG_DEBUG, "Client disconnected: ", fd);
    return IO_FAILED; // client切断
  }
  if (bytes_read == -1) {
    logfd(LOG_ERROR, "recv() failed: ", fd);
    return IO_FAILED; // 異常終了
  }
  parser.append_data(buffer, bytes_read);
  // parse 成功時, write fd監視開始
  return on_parse() ? IO_SUCCESS : IO_CONTINUE;
}

IOStatus Client::on_write() {
  if (!has_response()) {
    return IO_SUCCESS; // 送信可能なresponseがない
  }
  if (response_buffer.empty()) {
    response_buffer = response.get_next_response();
    response_sent = 0;
  }

  ssize_t bytes_sent = send(fd, response_buffer.c_str() + response_sent,
                            response_buffer.size() - response_sent, 0);
  if (bytes_sent <= 0) {
    logfd(LOG_ERROR, "send() failed: ", fd);
    return IO_FAILED; // 異常終了
  }
  response_sent += bytes_sent;
  if (response_sent >= response_buffer.size()) {
    response_buffer.clear();
    response.pop_response();
  }
  // continue: write続行, success: write完了
  return has_response() ? IO_CONTINUE : IO_SUCCESS;
}

bool Client::on_parse() {
  while (parser.parse()) {
    // trueの場合、requestの解析が完了しレスポンスを生成できる状態
    // request内でresponseクラスにresponseをpushしている
    request.handle_http_request();
    parser.clear(); // 常態をclearして, 再びheaderのparseを待機
  }
  return has_response();
}

bool Client::has_response() const {
  return (!response_buffer.empty() || response.has_next_response());
}

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
