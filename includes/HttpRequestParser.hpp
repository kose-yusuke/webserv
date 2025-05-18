#pragma once

#include "HttpRequest.hpp"
#include <sstream>
#include <string>
#include <vector>

class HttpRequest;

class HttpRequestParser {
public:
  HttpRequestParser(HttpRequest &http_request);
  ~HttpRequestParser();

  bool parse();                                      // データ解析
  void clear();                                      // 状態reset
  void append_data(const char *data, size_t length); // データ追加

private:
  enum ParseState {
    PARSE_HEADER, // headerの読み込み
    PARSE_BODY,   // bodyの読み込み
    PARSE_CHUNK,  // chunked body の読み込み
    PARSE_DONE    // 解析の終了
  };

  static const size_t k_max_request_line;
  static const size_t k_max_request_target;

  HttpRequest &request;
  ParseState parse_state;
  size_t body_size;
  std::vector<char> recv_buffer;

  void parse_header();
  void next_parse_state();
  void parse_body();
  void parse_chunked_body();

  bool parse_request_line(std::string &line);
  bool parse_header_line(std::string &line);

  void validate_request_content();
  bool check_framing_error();
  void validate_headers_content();

  void determine_connection_policy();

  bool is_valid_field_name_char(char c);
  void set_framing_error(int status = 400);

  HttpRequestParser(const HttpRequestParser &other);
  HttpRequestParser &operator=(const HttpRequestParser &other);
};
