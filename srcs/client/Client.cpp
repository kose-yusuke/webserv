
#include "Client.hpp"
#include "CgiSession.hpp"
#include "Logger.hpp"
#include <cstddef>
#include <sstream>
#include <stdexcept>

static const int k_default_timeout = 15;

// TODO: chunk_in_progress を反映させたlogicに修正
Client::Client(int clientfd, const VirtualHostRouter *router)
    : fd_(clientfd), state_(CLIENT_ALIVE), timeout_sec_(k_default_timeout),
      last_activity_(time(NULL)), transaction_(clientfd, router) {}

Client::~Client() {}

int Client::get_fd() const { return fd_; }

IOStatus Client::on_read() {
  LOG_DEBUG_FUNC();
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = recv(fd_, buffer, buf_size, 0);
  if (bytes_read <= 0) {
    transaction_.handle_client_abort();
    return IO_SHOULD_CLOSE;
  }
  update_activity();
  transaction_.append_raw_request(buffer, bytes_read);
  if (state_ == CLIENT_TIMED_OUT) {
    return IO_CONTINUE;
  }
  return transaction_.process_request_data(state_ == CLIENT_HALF_CLOSED);
}

IOStatus Client::on_write() {
  LOG_DEBUG_FUNC();
  if (state_ == CLIENT_HALF_CLOSED || !transaction_.has_response()) {
    return IO_WRITE_COMPLETE;
  }

  ResponseEntry *entry = transaction_.get_response();
  const std::string &buf = entry->buffer;
  size_t &offset = entry->offset;

  ssize_t bytes_sent = send(fd_, buf.c_str() + offset, buf.size() - offset, 0);
  if (bytes_sent <= 0) {
    transaction_.handle_client_abort();
    return IO_SHOULD_CLOSE;
  }
  update_activity();
  offset += bytes_sent;
  if (offset < buf.size()) {
    return IO_CONTINUE; // partial write
  }

  ConnectionPolicy conn_policy = entry->conn;
  bool is_chunk = entry->chunk_in_progress;
  transaction_.pop_response();

  IOStatus io_status = transaction_.decide_next_io(conn_policy, is_chunk);
  if (io_status == IO_SHOULD_SHUTDOWN) {
    state_ = CLIENT_HALF_CLOSED;
  }
  return io_status;
}

IOStatus Client::on_timeout() {
  if (state_ != CLIENT_ALIVE) {
    return IO_CONTINUE;
  }
  update_activity();
  transaction_.handle_client_timeout();
  state_ = CLIENT_TIMED_OUT;
  return IO_READY_TO_WRITE;
}

bool Client::is_timeout(time_t now) const {
  return (state_ == CLIENT_ALIVE && now - last_activity_ > timeout_sec_) ||
         state_ == CLIENT_CGI_TIMED_OUT;
}

bool Client::is_unresponsive(time_t now) const {
  return ((state_ != CLIENT_ALIVE) && now - last_activity_ > timeout_sec_);
}

void Client::set_cgi_timeout_status() {
  if (state_ != CLIENT_ALIVE) {
    return;
  }
  state_ = CLIENT_CGI_TIMED_OUT;
}

void Client::update_activity() { last_activity_ = time(NULL); }

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
