
#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <stdexcept>
#include <sys/socket.h>

Client::Client(int client_fd) : fd(client_fd), request(), response_sent(0) {}

Client::~Client() {}

IOStatus Client::on_read() {
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = recv(fd, buffer, sizeof(buf_size), 0);
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
    return true; // 送信可能なresponseがbufferにすでに待機
  }
  if (!request.is_header_received()) {
    size_t end = request_buffer.find("\r\n\r\n");
    if (end == std::string::npos) {
      return false; // header未受信
    }
    request.parse_header(request_buffer.substr(0, end + 4));
    request_buffer.erase(0, end + 4);
  }
  if (request.methodType_ == POST) {
    size_t bodySize = request.get_content_length();
    if (bodySize > 0 && request_buffer.size() < bodySize) {
      return false; // body未受信
    }
    request.parse_body(request_buffer.substr(0, bodySize));
    request_buffer.erase(0, bodySize);
  }
  response_buffer = HttpResponse::generate(request);
  response_sent = 0;
  request.clear();
  return true; // 解析完了
}

Client::Client() {}

Client::Client(const Client &other) { (void)other; }

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
