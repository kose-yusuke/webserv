
#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <stdexcept>
#include <sys/socket.h>

Client::Client(int clientFD) : fd_(clientFD), request_(), responseSent_(0) {}

Client::~Client() {}

bool Client::on_read() {
  const int bufSize = 1024;
  char buffer[bufSize];
  ssize_t bytesRead = recv(fd_, buffer, sizeof(bufSize), 0);
  if (bytesRead == 0) {
    std::cout << "Client disconnected: " << fd_ << std::endl;
    return false; // 正常終了
  }
  if (bytesRead == -1) {
    throw std::runtime_error("recv failed"); // 異常終了
  }
  requestBuffer_.append(buffer, bytesRead);
  return on_parse(); // 受信後に解析
}

bool Client::on_parse() {
  if (!responseBuffer_.empty()) {
    // responseが未送信の時はparseしない
    return false;
  }
  if (!request_.is_header_received()) {
    size_t end = requestBuffer_.find("\r\n\r\n");
    if (end == std::string::npos) {
      return false; // header未受信
    }
    request_.parse_header(requestBuffer_.substr(0, end + 4));
    requestBuffer_.erase(0, end + 4);
  }
  if (request_.methodType_ == POST) {
    size_t bodySize = request_.get_content_length();
    if (bodySize > 0 && requestBuffer_.size() < bodySize) {
      return false; // body未受信
    }
    request_.parse_body(requestBuffer_.substr(0, bodySize));
    requestBuffer_.erase(0, bodySize);
  }
  // 受信完了
  responseBuffer_ = HttpResponse::generate(request_);
  responseSent_ = 0;
  request_.clear();
  return true;
}

bool Client::on_write() {
  if (responseBuffer_.empty() && !on_parse()) {
    // 送信するresponseがない
    return false;
  }
  ssize_t bytesSent = send(fd_, responseBuffer_.c_str() + responseSent_,
                           responseBuffer_.size() - responseSent_, 0);
  if (bytesSent == 0 || bytesSent == -1) {
    throw std::runtime_error("send failed"); // 異常終了
  }
  responseSent_ += bytesSent;
  if (responseSent_ >= responseBuffer_.size()) {
    responseBuffer_.clear();
    responseSent_ = 0;
    return true; // 正常終了
  }
  return false; // 未送信部分があるため、再度sendが必要
}

// TODO: 未完了　
bool Client::on_error() { return true; }

Client::Client() {}

Client::Client(const Client &other) { (void)other; }

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
