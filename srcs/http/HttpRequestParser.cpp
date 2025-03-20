#include "HttpRequestParser.hpp"
#include "Utils.hpp"

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
  // TODO: Transfer-Encoding: chunked 対応
  // ex. state == PARSE_CHUNKED
  return (state == PARSE_DONE || state == PARSE_ERROR);
}

void HttpRequestParser::clear() {
  request.clear();
  state = PARSE_HEADER;
}

void HttpRequestParser::append_data(const char *data, size_t length) {
  buffer.append(data, length);
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

HttpRequestParser::ParseState HttpRequestParser::parse_header() {
  size_t end = buffer.find("\r\n\r\n");
  if (end == std::string::npos) {
    return PARSE_HEADER; // header未受信
  }

  std::istringstream iss(buffer.substr(0, end));
  std::string line;

  buffer.erase(0, end + 4);
  if (!std::getline(iss, line) || !parse_request_line(line)) {
    request.set_status_code(400);
    return PARSE_ERROR;
  }

  // TODO: validate_request_line() later あとで
  while (std::getline(iss, line) && !line.empty()) {
    log(LOG_DEBUG, "Parsing line: " + line);
    if (!parse_header_line(line)) {
      std::exit(1);
      return PARSE_ERROR;
    }
  }
  log(LOG_DEBUG, "Parse Success !!");
  // std::exit(1);
  // if (request.methodType == POST && request.get_content_length() > 0) {
  //   return (PARSE_BODY);
  // }
  return PARSE_DONE;
}

/**
 * Recipients of an invalid request-line SHOULD respond with either a
 * 400 (Bad Request) error or a 301 (Moved Permanently) redirect with
 * the request-target properly encoded.
 *
 * HTTP does not place a predefined limit on the length of a
 * request-line, as described in Section 2.5.  A server that receives a
 * method longer than any that it implements SHOULD respond with a 501
 * (Not Implemented) status code.
 *
 * A server that receives a request-target longer than any URI it wishes
 * to parse MUST respond with a 414 (URI Too Long) status code
 * (see Section 6.5.12 of [RFC7231]).
 */
bool HttpRequestParser::parse_request_line(std::string &line) {
  if (line.empty() || line.size() > MAX_REQUEST_LINE || std::isspace(line[0])) {
    return false;
  }

  std::istringstream request_iss(line);
  if (!(request_iss >> request.method >> request.path >> request.version)) {
    log(LOG_ERROR, "Failed to parse request line: " + line);
    return false;
  }

  std::string ext;
  if (request_iss >> ext) {
    log(LOG_DEBUG, "Extra characters (" + ext + ") in request line");
    return false;
  }

  log(LOG_DEBUG, "Parsed Request - Method: " + request.method + ", Path: " +
                     request.path + ", Version: " + request.version);
  return true;
}

bool HttpRequestParser::parse_header_line(std::string &line) {

  if (std::isspace(line[0])) {
    log(LOG_ERROR, "Invalid leading space in header line: " + line);
    return false;
  }

  size_t pos = line.find(": ");
  if (pos == std::string::npos) {
    log(LOG_ERROR, "Failed to parse header line: " + line);
    return false;
  }

  std::string key = trim(line.substr(0, pos));
  std::string value = trim(line.substr(pos + 2));
  if (!request.add_header(key, value)) {
    log(LOG_ERROR, "Duplicate header found: " + key);
    return false;
  }

  log(LOG_DEBUG, "Parsed - Key: " + key + ": " + request.headers[key]);
  return true;
}

// bool HttpRequestParser::validate_request_line() {
//   if (request.method.empty() || request.path.empty() ||
//   request.version.empty()) {
//       log(LOG_ERROR, "Request line contains invalid values\n");
//       return false;
//   }

//   static const std::set<std::string> supported_methods = {
//       "GET", "POST", "HEAD", "PUT", "DELETE", "OPTIONS", "TRACE"
//   };
//   if (supported_methods.count(request.method) == 0) {
//       log(LOG_ERROR, "Unsupported HTTP method: " + request.method + "\n");
//       request.set_status_code(501);
//       return false;
//   }

//   if (request.version != "HTTP/1.1") {
//       log(LOG_ERROR, "Unsupported HTTP version: " + request.version + "\n");
//       request.set_status_code(505);
//       return false;
//   }

//   return true;
// }
