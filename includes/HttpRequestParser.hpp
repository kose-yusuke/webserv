#pragma once

#include "HttpRequest.hpp"
#include <sstream>
#include <string>
#include <vector>

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
  // std::string buffer; // recv用の受信buffer
  std::vector<char> recv_buffer;

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
