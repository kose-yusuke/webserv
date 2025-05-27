#pragma once

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include <cstddef>

class CgiSession;

class HttpTransaction {
public:
  HttpTransaction();
  ~HttpTransaction();

  bool parse(char *buffer, size_t len); // ← parser_使って更新
  bool is_complete();
  void handle_request(); // ← request_.handle_http_request()
  void handle_cgi();     // ← cgi_.handle_cgi_request(...)
  HttpResponse &get_response();
  // etc...

private:
  HttpRequestParser parser_;
  HttpRequest request_;
  HttpResponse response_;

  // どっちにするか悩み中
  CgiSession *cgi_;
  CgiSession cgi_;


  HttpTransaction(const HttpTransaction &other);
  HttpTransaction &operator=(const HttpTransaction &other);
};
