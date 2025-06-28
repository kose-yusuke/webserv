#pragma once

#include "ResponseTypes.hpp"
#include <string>
#include <utility>
#include <vector>

class HttpResponse;
class CgiParser;

/*
CgiResponseBuilder:
CgiParser から得られた情報を元に、HttpResponseを組み立てる
*/
class CgiResponseBuilder {
public:
  CgiResponseBuilder();
  ~CgiResponseBuilder();

  void apply(CgiParser &parser);
  void set_connection_policy(ConnectionPolicy policy);
  void build_response(HttpResponse &response, bool eof);
  void build_error_response(HttpResponse &response, int status_code);
  void build_error_response(HttpResponse &response, int status_code,
                            ConnectionPolicy policy);
  bool is_sent() const;

private:
  int status_code_;
  std::vector<std::pair<std::string, std::string> > headers_;
  std::vector<char> body_;
  ConnectionPolicy conn_policy_;
  bool is_chunked_;
  bool is_header_sent_;
  bool is_response_sent_;
};
