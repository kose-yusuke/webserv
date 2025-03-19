
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
  request_buffer.append(buffer, bytes_read);
  return on_parse() ? IO_SUCCESS : IO_CONTINUE;
}

IOStatus Client::on_write() {
  if (!on_parse()) {
    return IO_SUCCESS; // 送信可能なresponseがないため, write側監視のみ終了
  }
  ssize_t bytes_sent = send(fd, response_buffer.c_str() + response_sent,
                            response_buffer.size() - response_sent, 0);
  if (bytes_sent == 0 || bytes_sent == -1) {
    std::cerr << "Error: send failed: " << fd << " (" << bytes_sent << ")\n";
    return IO_FAILED; // 異常終了
  }
  response_sent += bytes_sent;
  if (response_sent >= response_buffer.size()) {
    response_buffer.clear();
    response_sent = 0;
    // 次に送信可能なbufがあれば継続し、なければwrite側監視のみ終了
    return on_parse() ? IO_CONTINUE : IO_SUCCESS;
  }
  return IO_CONTINUE; // 未送信部分があるため、再度sendが必要
}

bool Client::on_parse() {
  if (!response_buffer.empty()) {
    // 送信可能なresponseがbufferにすでに待機済み
    return true;
  }
  if (parser.parse(request_buffer)) {
    // trueの場合、requestの解析が完了しレスポンスを生成できる状態
    // PARSE_ERRORもerror responseを用意できる
    // response_buffer = HttpResponse::generate(request, server_fd);
    parser.clear(); // 常態をclearして, 再びheader待機状態に
    response_sent = 0;
    return true;
  }
  return false; // responseを用意できなかった
}

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
