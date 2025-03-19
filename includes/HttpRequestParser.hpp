#pragma once
#include "HttpRequest.hpp"
#include <sstream>

class HttpRequestParser {
public:
  HttpRequestParser(HttpRequest &http_request);
  ~HttpRequestParser();

  bool parse(std::string &buffer);
  void clear(); // 状態reset

private:
  enum ParseState {
    PARSE_HEADER, // headerの読み込み
    PARSE_BODY,   // bodyの読み込み
    PARSE_ERROR,  // 解析の異常終了
    PARSE_DONE    // 解析の正常終了
  };

  HttpRequest &request;
  ParseState state; // 解析状態

  ParseState parse_header(std::string &buffer);
  ParseState parse_body(std::string &buffer);

  HttpRequestParser(const HttpRequestParser &other);
  HttpRequestParser &operator=(const HttpRequestParser &other);
};
