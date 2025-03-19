#include "HttpRequestParser.hpp"

HttpRequestParser::HttpRequestParser(HttpRequest &http_request)
    : request(http_request), state(PARSE_HEADER) {}

HttpRequestParser::~HttpRequestParser() {}

bool HttpRequestParser::parse() {
  if (buffer.empty()) {
    return state;
  }
  if (state == PARSE_HEADER) {
    state = parse_header();
  }
  if (state == PARSE_BODY) {
    state = parse_body();
  }
  return (state == PARSE_DONE || state == PARSE_ERROR);
}

void HttpRequestParser::clear() {
  request.clear();
  state = PARSE_HEADER;
}

void HttpRequestParser::append_data(const char *data, size_t length) {
  buffer.append(data, length);
}

HttpRequestParser::ParseState HttpRequestParser::parse_header() {
  size_t end = buffer.find("\r\n\r\n");
  if (end == std::string::npos) {
    return PARSE_HEADER; // header未受信
  }

  std::istringstream stream(buffer.substr(0, end + 4));
  std::string line;
  buffer.erase(0, end + 4);

  // TODO: 流れのみ。細かくは後で修正
  while (std::getline(stream, line) && !line.empty()) {
    size_t pos = line.find(": ");
    if (pos != std::string::npos) {
      request.headers[line.substr(0, pos)] = line.substr(pos + 2);
    }
  }
  if (request.methodType == POST && request.get_content_length() > 0) {
    return (PARSE_BODY);
  }
  return PARSE_DONE;
}

HttpRequestParser::ParseState HttpRequestParser::parse_body() {
  size_t body_size = request.get_content_length();
  if (body_size == 0) {
    return PARSE_DONE; // bodyなし
  }
  if (body_size > 0 && buffer.size() < body_size) {
    return PARSE_BODY; // body未受信
  }
  if (body_size > 10 * 1024 * 1024) {
    buffer.erase(0, body_size); // TODO: 消していいかを確認
    return PARSE_ERROR;
  }
  request.body.append(buffer.substr(0, body_size));
  buffer.erase(0, body_size);
  return PARSE_DONE;
}

HttpRequestParser &
HttpRequestParser::operator=(const HttpRequestParser &other) {
  (void)other;
  return *this;
}

// parse headerで読み込み、validate headerでvalidationをする予定
