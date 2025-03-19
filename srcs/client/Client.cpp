
#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <stdexcept>
#include <sys/socket.h>

Client::Client(int clientfd, int serverfd)
    : fd(clientfd), server_fd(serverfd), request(serverfd), parser(request),
      response_sent(0) {}

Client::~Client() {}

IOStatus Client::on_read() {
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes_read == 0) {
    std::cout << "Client disconnected: " << fd << std::endl;
    return IO_FAILED; // client切断
  }
  if (bytes_read == -1) {
    std::cerr << "Error: recv failed: " << fd << std::endl;
    return IO_FAILED; // 異常終了
  }
  parser.append_data(buffer, bytes_read);
  // parse 成功時, write fd監視開始
  return on_parse() ? IO_SUCCESS : IO_CONTINUE;
}

IOStatus Client::on_write() {
  if (!on_parse()) {
    return IO_SUCCESS; // 送信可能なresponseがない時, writeを終了
  }
  std::string &response = response_queue.front();
  ssize_t bytes_sent = send(fd, response.c_str() + response_sent,
                            response.size() - response_sent, 0);
  if (bytes_sent == 0 || bytes_sent == -1) {
    std::cerr << "Error: send failed: " << fd << " (" << bytes_sent << ")\n";
    return IO_FAILED; // 異常終了
  }
  response_sent += bytes_sent;
  if (response_sent >= response.size()) {
    response_queue.pop();
    response_sent = 0;
  }
  return response_queue.empty() ? IO_SUCCESS : IO_CONTINUE;
}

bool Client::on_parse() {
  // 複数のresponseを生成できる
  while (parser.parse()) {
    // trueの場合、requestの解析が完了しレスポンスを生成できる状態
    std::string response = request.handle_http_request(0, 0, 0);
    response_queue.push(response);
    parser.clear(); // 常態をclearして, 再びheader待機状態に
  }
  return !response_queue.empty(); // 待機済みのresponseがあるか確認
}

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
