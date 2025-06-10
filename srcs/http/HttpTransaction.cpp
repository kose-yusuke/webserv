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

// parserのbufferにraw dataを蓄積
void HttpTransaction::append_data(const char *raw, size_t length) {
  LOG_DEBUG_FUNC();
  parser_.append_data(raw, length);
}

// parse + response生成
void HttpTransaction::process_data() {
  LOG_DEBUG_FUNC();
  if (cgi_.is_cgi_active()) {
    return;
  }
  bool keep_alive = true;
  while (keep_alive && parser_.parse()) {
    request_.handle_http_request(); // responseを生成し、response queueに積む
    if (cgi_.is_cgi_active()) {
      return;
    }
    keep_alive = request_.get_connection_policy() == CP_KEEP_ALIVE;
    parser_.clear();
  }
}

// is_half_closed時のparse + `Connection: close` header検知
bool HttpTransaction::should_close() {
  LOG_DEBUG_FUNC();
  if (cgi_.is_cgi_active()) {
    return false;
  }
  while (parser_.parse()) {
    if (request_.get_connection_policy() == CP_WILL_CLOSE) {
      return true; // trueの際は、IO_SHOULD_CLOSEになる
    }
    parser_.clear();
  }
  return false;
}

IOStatus HttpTransaction::decide_io_after_write(ConnectionPolicy conn_policy,
                                                ResponseType response_type) {
  LOG_DEBUG_FUNC();

  switch (response_type) {
  case CHUNK:
    return has_response() ? IO_CONTINUE : IO_WRITE_COMPLETE;
  case CHUNK_LAST:
    reset_cgi_session();
    process_data();
    break;
  case NORMAL:
  default:
    process_data();
  }

  switch (conn_policy) {
  case CP_WILL_CLOSE:
    return IO_SHOULD_CLOSE;
  case CP_MUST_CLOSE:
    return IO_SHOULD_SHUTDOWN;
  case CP_KEEP_ALIVE:
  default:
    return has_response() ? IO_CONTINUE : IO_WRITE_COMPLETE;
  }
}

void HttpTransaction::reset_cgi_session() {
  LOG_DEBUG_FUNC();
  cgi_.reset();
  parser_.clear();
}

void HttpTransaction::handle_client_timeout() {
  LOG_DEBUG_FUNC();
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
  LOG_DEBUG_FUNC();
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
