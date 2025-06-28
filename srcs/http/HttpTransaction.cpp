#include "HttpTransaction.hpp"
#include "CgiSession.hpp"

// NOTE: HttpTransaction は HTTP処理状態を制御
// ソケット（fd）とコネクションの生死は clientの管轄
// CGIの応答遅延など → HttpTransaction あるいは CgiSession に責務を持たせる
HttpTransaction::HttpTransaction(int fd, const VirtualHostRouter *router)
    : client_fd_(fd), response_(), request_(fd, router, response_),
      parser_(request_) {}

HttpTransaction::~HttpTransaction() {}

// parserのbufferにraw dataを蓄積
void HttpTransaction::append_data(const char *raw, size_t length) {
  LOG_DEBUG_FUNC();
  parser_.append_data(raw, length);
}

// parse + response生成
void HttpTransaction::process_data() {
  LOG_DEBUG_FUNC();
  if (request_.has_cgi_session()) {
    process_cgi_session();
    return;
  }
  bool keep_alive = true;
  while (keep_alive && parser_.parse()) {
    request_.handle_http_request(); // responseを生成し、response queueに積む
    if (request_.has_cgi_session()) {
      process_cgi_session();
      return;
    }
    keep_alive = request_.get_connection_policy() == CP_KEEP_ALIVE;
    parser_.clear();
  }
}

void HttpTransaction::process_cgi_session() {
  LOG_DEBUG_FUNC();
  CgiSession *session = request_.get_cgi_session();

  session->build_response(response_); // まだdataがなければ何もしない
  if (session->is_failed() || session->is_session_completed()) {
    bool keep_alive = request_.get_connection_policy() == CP_KEEP_ALIVE;
    request_.clear_cgi_session();
    parser_.clear(); // 次のリクエストへ
    if (keep_alive) {
      process_data();
    }
    return;
  }
}

// is_half_closed時のparse + `Connection: close` header検知
bool HttpTransaction::should_close() {
  LOG_DEBUG_FUNC();
  if (request_.has_cgi_session()) {
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

IOStatus HttpTransaction::decide_io_after_write(ConnectionPolicy conn_policy) {
  LOG_DEBUG_FUNC();
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

void HttpTransaction::handle_client_timeout() {
  LOG_DEBUG_FUNC();
  if (request_.has_cgi_session()) {
    CgiSession *session = request_.get_cgi_session();
    session->build_error_response(response_, 408, CP_MUST_CLOSE);
    if (session->is_failed() || session->is_session_completed()) {
      request_.clear_cgi_session();
    } else {
      session->mark_client_dead();
    }
  } else {
    response_.generate_timeout_response();
  }
}

void HttpTransaction::handle_client_abort() {
  LOG_DEBUG_FUNC();
  if (request_.has_cgi_session()) {
    CgiSession *session = request_.get_cgi_session();
    if (session->is_failed() || session->is_session_completed()) {
      request_.clear_cgi_session();
    } else {
      session->mark_client_dead();
    }
  }
}

bool HttpTransaction::has_response() const {
  return (response_.has_response());
}

ResponseEntry *HttpTransaction::get_response() {
  return response_.get_front_response();
}

void HttpTransaction::pop_response() { response_.pop_front_response(); }

HttpTransaction &HttpTransaction::operator=(const HttpTransaction &other) {
  (void)other;
  return *this;
}
