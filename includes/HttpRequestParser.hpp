#pragma once

#include "HttpRequest.hpp"
#include <sstream>
#include <string>

class HttpRequestParser {
public:
  HttpRequestParser(HttpRequest &http_request);
  ~HttpRequestParser();

  bool parse();
  void clear(); // 状態reset

  void append_data(const char *data, size_t length); // データ追加

private:
  enum ParseState {
    PARSE_HEADER, // headerの読み込み
    PARSE_BODY,   // bodyの読み込み
    PARSE_ERROR,  // 解析の異常終了
    PARSE_DONE    // 解析の正常終了
  };

  HttpRequest &request;
  ParseState state;   // 解析状態
  std::string buffer; // recv用の受信buffer

  ParseState parse_header();
  ParseState parse_body();

  HttpRequestParser(const HttpRequestParser &other);
  HttpRequestParser &operator=(const HttpRequestParser &other);
};
