#include "CgiResponseBuilder.hpp"
#include "CgiParser.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"
#include <sstream>
#include <vector>

CgiResponseBuilder::CgiResponseBuilder()
    : status_code_(0), headers_(), body_(), conn_policy_(CP_KEEP_ALIVE),
      is_chunked_(false), is_header_sent_(false), is_response_sent_(false) {}

CgiResponseBuilder::~CgiResponseBuilder() {}

void CgiResponseBuilder::apply(CgiParser &parser) {
  status_code_ = parser.get_status_code();
  headers_ = parser.get_parsed_headers();
  body_ = parser.take_body();

  // Transfer-Encoding: chunked があれば chunk ON
  is_chunked_ = false;
  for (size_t i = 0; i < headers_.size(); ++i) {
    if (to_lower(headers_[i].first) == "transfer-encoding" &&
        to_lower(headers_[i].second) == "chunked") {
      is_chunked_ = true;
      break;
    }
  }
}

void CgiResponseBuilder::set_connection_policy(ConnectionPolicy policy) {
  conn_policy_ = policy;
}

void CgiResponseBuilder::build_response(HttpResponse &response, bool eof) {
  if (is_response_sent_) {
    log(LOG_WARNING, "CgiResponseBuilder: attempt to send duplicate response");
    return;
  }
  if (status_code_ == 500) {
    response.generate_error_response(500, conn_policy_);
    is_response_sent_ = true;
    return;
  }

  if (is_chunked_) {
    if (!is_header_sent_) {
      response.generate_chunk_response_header(status_code_, headers_,
                                              conn_policy_);
      is_header_sent_ = true;
    }
    if (!body_.empty()) {
      response.generate_chunk_response_body(body_);
      body_.clear();
    }
    if (eof) {
      response.generate_chunk_response_last(conn_policy_);
      is_response_sent_ = true;
    }
  } else {
    response.generate_response(status_code_, headers_, body_, conn_policy_);
    is_response_sent_ = true;
  }
}

void CgiResponseBuilder::build_error_response(HttpResponse &response,
                                              int status_code) {
  if (is_response_sent_) {
    return;
  }
  if (is_chunked_ && is_header_sent_) {
    response.generate_chunk_response_last(conn_policy_);
  } else {
    response.generate_error_response(status_code, conn_policy_);
  }
  is_response_sent_ = true;
}

void CgiResponseBuilder::build_error_response(HttpResponse &response,
                                              int status_code,
                                              ConnectionPolicy policy) {
  if (is_response_sent_) {
    return;
  }
  if (is_chunked_ && is_header_sent_) {
    response.generate_chunk_response_last(policy);
  } else {
    response.generate_error_response(status_code, policy);
  }
  is_response_sent_ = true;
}

bool CgiResponseBuilder::is_sent() const { return is_response_sent_; }
