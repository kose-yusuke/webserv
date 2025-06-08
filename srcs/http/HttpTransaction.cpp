#include "HttpTransaction.hpp"
#include "CgiSession.hpp"

// NOTE: HttpTransaction は HTTP処理状態を制御
// ソケット（fd）とコネクションの生死は clientの管轄
// CGIの応答遅延など → HttpTransaction あるいは CgiSession に責務を持たせる
HttpTransaction::HttpTransaction(int fd, const VirtualHostRouter *router)
    : response_(), request_(router, response_), parser_(request_),
      cgi_(request_, response_, fd) {
  request_.set_cgi_session(&cgi_);
}

HttpTransaction::~HttpTransaction() {}

void HttpTransaction::append_raw_request(const char *raw, size_t length) {
  parser_.append_data(raw, length);
}

IOStatus HttpTransaction::process_request_data(bool is_half_closed) {
  LOG_DEBUG_FUNC();
  while (!cgi_.is_cgi_active() && parser_.parse()) {

    if (!is_half_closed) {
      request_.handle_http_request(); // response の生成が走る
    }
    if (request_.get_connection_policy() != CP_KEEP_ALIVE ||
        cgi_.is_cgi_active()) {
      break;
    }
    parser_.clear();
  }
  if (is_half_closed && request_.get_connection_policy() == CP_WILL_CLOSE) {
    return IO_SHOULD_CLOSE;
  }
  return !is_half_closed && has_response() ? IO_READY_TO_WRITE : IO_CONTINUE;
}

IOStatus HttpTransaction::decide_next_io(ConnectionPolicy conn, bool is_chunk) {

  if (is_chunk) {

    return has_response() ? IO_CONTINUE : IO_WRITE_COMPLETE;
  }

  switch (conn) {
  case CP_WILL_CLOSE:
    return IO_SHOULD_CLOSE;
  case CP_MUST_CLOSE:
    return IO_SHOULD_SHUTDOWN;
  case CP_KEEP_ALIVE:
  default:
    return has_response() ? IO_CONTINUE : IO_WRITE_COMPLETE;
  }
}

void HttpTransaction::handle_client_timeout() {
  if (!cgi_.is_cgi_active()) {
    response_.generate_timeout_response();
  } else {
    cgi_.on_client_timeout();
  }
}

void HttpTransaction::handle_client_abort() {
  if (!cgi_.is_cgi_active()) {
    return;
  }
  cgi_.on_client_abort();
}

bool HttpTransaction::has_response() const {
  return (response_.has_next_response());
}

ResponseEntry *HttpTransaction::get_response() {
  return response_.get_front_response();
}

void HttpTransaction::pop_response() { response_.pop_front_response(); }

HttpTransaction &HttpTransaction::operator=(const HttpTransaction &other) {
  (void)other;
  return *this;
}
