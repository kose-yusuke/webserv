
#include "Client.hpp"
#include "Logger.hpp"
#include <cstddef>
#include <sstream>
#include <stdexcept>

// TODO: timeoutのconfigに基づく設定はあとで考慮する
Client::Client(int clientfd, const VirtualHostRouter *router)
    : fd_(clientfd), client_state_(CLIENT_ALIVE), timeout_sec_(15),
      last_activity_(time(NULL)), response_(), request_(router, response_),
      cgi_(response_, request_), parser_(request_), current_entry_(NULL), 
      response_sent_(0) 
      {
        request_.set_cgi_handler(&cgi_);
      }

Client::~Client() {}

int Client::get_fd() const { return fd_; }

IOStatus Client::on_read() {
  LOG_DEBUG_FUNC();
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = recv(fd_, buffer, sizeof(buffer), 0);
  if (bytes_read == -1 || bytes_read == 0) {
    return IO_SHOULD_CLOSE;
  }

  std::string debug(buffer, bytes_read);

  update_activity();
  parser_.append_data(buffer, bytes_read);

  if (client_state_ == CLIENT_TIMED_OUT) {
    return IO_CONTINUE; // Time-outのレスポンス送信中。clientからの入力は無視する
  }
  if (client_state_ == CLIENT_HALF_CLOSED) {
    return on_half_close();
  }
  return on_parse();
}

IOStatus Client::on_write() {
  LOG_DEBUG_FUNC();
  if (client_state_ == CLIENT_HALF_CLOSED || !has_response()) {
    return IO_WRITE_COMPLETE;
  }
  if (!current_entry_) {
    current_entry_ = response_.get_front_response();
    response_sent_ = 0;
  }

  std::string &send_buffer = current_entry_->buffer;
  ssize_t bytes_sent = send(fd_, send_buffer.c_str() + response_sent_,
                            send_buffer.size() - response_sent_, 0);
  if (bytes_sent == -1 || bytes_sent == 0) {
    return IO_SHOULD_CLOSE;
  }
  update_activity();

  response_sent_ += bytes_sent;
  if (response_sent_ < current_entry_->buffer.size()) {
    return IO_CONTINUE; // 今回のsendで、response全体を送信できなかった
  }

  ConnectionPolicy conn_policy = current_entry_->conn; // response送信完了
  response_.pop_front_response();
  current_entry_ = 0;

  if (conn_policy == CP_WILL_CLOSE) {
    return IO_SHOULD_CLOSE;
  } else if (conn_policy == CP_MUST_CLOSE) {
    client_state_ = CLIENT_HALF_CLOSED;
    return IO_SHOULD_SHUTDOWN;
  }
  return has_response() ? IO_CONTINUE : IO_WRITE_COMPLETE;
}

// stateの更新と、timeout responseの準備
IOStatus Client::on_timeout() {
  if (client_state_ != CLIENT_ALIVE) {
    return IO_CONTINUE;
  }
  client_state_ = CLIENT_TIMED_OUT;
  current_entry_ = 0;
  response_.generate_timeout_response();
  update_activity();
  return IO_READY_TO_WRITE; // monitor_write()を促す
}

IOStatus Client::on_half_close() {
  LOG_DEBUG_FUNC();
  while (parser_.parse()) {
    if (request_.get_connection_policy() == CP_WILL_CLOSE) {
      log(LOG_DEBUG, "Graceful shutdown confirmed from client.");
      return IO_SHOULD_CLOSE;
    }
    parser_.clear();
  }
  return IO_CONTINUE;
}

IOStatus Client::on_parse() {
  LOG_DEBUG_FUNC();
  while (parser_.parse()) {
    // trueの場合、requestの解析が完了しレスポンスを生成できる状態
    // request内でresponseクラスにresponseをpushしている
    request_.handle_http_request();
    if (request_.get_connection_policy() != CP_KEEP_ALIVE) {
      break;
    }
    parser_.clear(); // 状態をclearして, 再びheaderのparseを待機
  }
  return has_response() ? IO_READY_TO_WRITE : IO_CONTINUE;
}

bool Client::has_response() const {
  // 送信中の entry または これから送信を開始できる is_ready な entry がある
  return ((current_entry_ && current_entry_->is_ready) ||
          response_.has_next_response());
}

void Client::update_activity() { last_activity_ = time(NULL); }

bool Client::is_timeout(time_t now) const {
  return (client_state_ == CLIENT_ALIVE && now - last_activity_ > timeout_sec_);
}

bool Client::is_unresponsive(time_t now) const {
  return ((client_state_ == CLIENT_TIMED_OUT ||
           client_state_ == CLIENT_HALF_CLOSED) &&
          now - last_activity_ > timeout_sec_);
}

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
