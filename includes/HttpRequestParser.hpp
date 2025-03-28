#pragma once

#include "HttpRequest.hpp"
#include <sstream>
#include <string>

class HttpRequestParser {
public:
  HttpRequestParser(HttpRequest &http_request);
  ~HttpRequestParser();

  bool parse();

  void clear();                                      // 状態reset
  void append_data(const char *data, size_t length); // データ追加

private:
  enum ParseState {
    PARSE_HEADER, // headerの読み込み
    PARSE_BODY,   // bodyの読み込み
    PARSE_CHUNK,  // chunked body 対応
    PARSE_ERROR,  // 解析の異常終了
    PARSE_DONE    // 解析の正常終了
  };

  static const size_t k_max_request_line;
  static const size_t k_max_request_target;

  HttpRequest &request;
  ParseState parse_state; // 解析状態
  size_t body_size;
  std::string buffer; // recv用の受信buffer

  ParseState parse_header();
  ParseState next_parse_state() const;
  ParseState parse_body();
  ParseState parse_chunked_body();

  bool parse_request_line(std::string &line);
  bool parse_header_line(std::string &line);

  bool validate_request_content();
  bool validate_headers_content();

  HttpRequestParser(const HttpRequestParser &other);
  HttpRequestParser &operator=(const HttpRequestParser &other);
};

/**
 * Recipients of an invalid request-line SHOULD respond with either a
 * 400 (Bad Request) error or a 301 (Moved Permanently) redirect with
 * the request-target properly encoded.
 *
 * HTTP does not place a predefined limit on the length of a
 * request-line, as described in Section 2.5.
 *
 * A server that receives a method longer than any that it implements SHOULD
 * respond with a 501 (Not Implemented) status code.
 *
 * A server that receives a request-target longer than any URI it wishes
 * to parse MUST respond with a 414 (URI Too Long) status code
 * (see Section 6.5.12 of [RFC7231]).
 */

/*

 // std::exit(1);
   // if (request.methodType == POST && request.get_content_length() > 0) {
   //   return (PARSE_BODY);
   // }
*/

// bool HttpRequestParser::validate_request_line() {
//   if (request.method.empty() || request.path.empty() ||
//   request.version.empty()) {
//       log(LOG_ERROR, "Request line contains invalid values\n");
//       return false;
//   }

//   if (supported_methods.count(request.method) == 0) {
//       log(LOG_ERROR,  + "\n");
//
//   }

//   if (request.version != "HTTP/1.1") {
//       log(LOG_ERROR, "Unsupported HTTP version: " + request.version + "\n");
//       request.set_status_code(505);
//       return false;
//   }

//   return true;
// }
