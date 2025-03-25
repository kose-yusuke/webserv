#include "HttpRequestParser.hpp"
#include "Utils.hpp"
#include <set>

const size_t HttpRequestParser::k_max_request_line = 10000;
const size_t HttpRequestParser::k_max_request_target = 2048;

static const char *methods_arr[] = {"GET",    "HEAD",    "POST",    "PUT",
                                    "DELETE", "CONNECT", "OPTIONS", "TRACE"};

static const std::set<std::string> supported_methods(
    methods_arr, methods_arr + sizeof(methods_arr) / sizeof(methods_arr[0]));

HttpRequestParser::HttpRequestParser(HttpRequest &http_request)
    : request(http_request), parse_state(PARSE_HEADER) {}

HttpRequestParser::~HttpRequestParser() {}

bool HttpRequestParser::parse() {
  LOG_DEBUG_FUNC();
  if (buffer.empty()) {
    return parse_state;
  }
  if (parse_state == PARSE_HEADER) {
    parse_state = parse_header();
  }
  if (parse_state == PARSE_BODY) {
    parse_state = parse_body();
  }
  return (parse_state == PARSE_DONE || parse_state == PARSE_ERROR);
}

void HttpRequestParser::clear() {
  LOG_DEBUG_FUNC();
  request.clear();
  parse_state = PARSE_HEADER;
  body_size = 0;
}

void HttpRequestParser::append_data(const char *data, size_t length) {
  LOG_DEBUG_FUNC();
  buffer.append(data, length);
}

HttpRequestParser::ParseState HttpRequestParser::parse_header() {
  LOG_DEBUG_FUNC();
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
  if (!validate_request_content()) {
    return PARSE_ERROR;
  }
  while (std::getline(iss, line) && !line.empty()) {
    if (!parse_header_line(line)) {
      request.set_status_code(400);
      return PARSE_ERROR;
    }
  }
  if (!validate_headers_content()) {
    return PARSE_ERROR;
  }
  log(LOG_DEBUG, "Parse Success !!");
  return next_parse_state();
}

HttpRequestParser::ParseState HttpRequestParser::next_parse_state() const {
  if (request.is_in_headers("Transfer-Encoding")) {
    return PARSE_CHUNK;
  }
  if (request.is_in_headers("Content-length")) {
    return PARSE_BODY;
  }
  return PARSE_DONE;
}

HttpRequestParser::ParseState HttpRequestParser::parse_body() {
  LOG_DEBUG_FUNC();
  if (body_size == 0) {
    return PARSE_DONE; // bodyなし
  }
  if (body_size > 0 && buffer.size() < body_size) {
    return PARSE_BODY; // body未受信
  }
  request.body.append(buffer.substr(0, body_size));
  buffer.erase(0, body_size);
  return PARSE_DONE; // body受信完了
}

HttpRequestParser::ParseState HttpRequestParser::parse_chunked_body() {
  LOG_DEBUG_FUNC();
  size_t chunk_size = 0;
  while (true) {
    size_t pos = buffer.find("\r\n");
    if (pos == std::string::npos) {
      return PARSE_CHUNK;
    }

    std::string size_str = buffer.substr(0, pos);
    try {
      chunk_size = parse_hex(size_str);
    } catch (const std::exception &e) {
      request.set_status_code(400);
      return PARSE_ERROR;
    }
    if (chunk_size == 0) {
      break;
    }

    size_t chunk_start = pos + 2;
    size_t chunk_end = chunk_start + chunk_size + 2;
    if (buffer.size() < chunk_end) {
      return PARSE_CHUNK;
    }

    std::string chunk = buffer.substr(chunk_start, chunk_size);
    request.body.append(chunk);
    buffer.erase(0, chunk_end);
  }

  if (buffer.size() < 5) {
    return PARSE_CHUNK;
  }
  if (buffer.substr(0, 5) != "0\r\n\r\n") {
    request.set_status_code(400);
    return PARSE_ERROR;
  }
  buffer.erase(0, 5);
  return PARSE_DONE;
}

bool HttpRequestParser::parse_request_line(std::string &line) {
  LOG_DEBUG_FUNC();

  if (line.empty() || line.size() > k_max_request_line ||
      std::isspace(line[0])) {
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
  // LOG_DEBUG_FUNC();
  if (line.size() > k_max_request_line || std::isspace(line[0])) {
    log(LOG_ERROR, "Invalid header field: " + line);
    return false;
  }

  size_t pos = line.find(":");
  if (pos == std::string::npos || pos == 0) {
    log(LOG_ERROR, "Failed to parse header line: " + line);
    return false;
  }
  if (std::isspace(line.at(pos - 1))) {
    log(LOG_ERROR, "Invalid whitespace between field-name and colon: " + line);
    return false;
  }

  std::string key = line.substr(0, pos); // keyはtrimしない
  std::string value = trim(line.substr(pos + 1));
  if (!request.add_header(key, value)) {
    log(LOG_ERROR, "Duplicate header found: " + key);
    // TODO: exceptionあるので注意 ex. CSV, or known exception
    return false;
  }

  log(LOG_DEBUG, "Parsed - Key: " + key + ": " + request.headers[key]);
  return true;
}

bool HttpRequestParser::validate_request_content() {
  LOG_DEBUG_FUNC();
  if (request.method.empty() || request.path.empty() ||
      request.version.empty()) {
    log(LOG_DEBUG, "Failed to parse request line (empty value)");
    request.set_status_code(400);
    return false;
  }

  if (supported_methods.find(request.method) == supported_methods.end()) {
    log(LOG_DEBUG, "Unsupported HTTP method: " + request.method);
    request.set_status_code(501);
    return false;
  }

  // TODO: Request url が NG文字を含む
  // request.set_status_code(400);

  if (request.path.size() > k_max_request_target) {
    log(LOG_DEBUG, "Request-target is too long");
    request.set_status_code(414);
    return false;
  }

  if (request.version != "HTTP/1.1") {
    log(LOG_ERROR, "Unsupported HTTP version: " + request.version);
    request.set_status_code(505);
    return false;
  }
  return true;
}

bool HttpRequestParser::validate_headers_content() {
  LOG_DEBUG_FUNC();
  if (request.method != "POST") {
    return false;
  }

  // Transfer-Encoding  あり. but not chunked
  if (request.is_in_headers("Transfer-Encoding") &&
      request.get_value_from_headers("Transfer-Encoding") != "chunked") {
    request.set_status_code(501);
    return false;
  }

  // Transfer-Encoding も Content-length もない POST
  if (!request.is_in_headers("Transfer-Encoding") &&
      !request.is_in_headers("Content-length")) {
    request.set_status_code(400);
    return false;
  }

  // TODO: status code 確認
  // Transfer-Encoding も Content-length ON (両立しない)
  if (request.is_in_headers("Transfer-Encoding") &&
      request.is_in_headers("Content-length")) {
    request.set_status_code(400);
    return false;
  }

  // Request body larger than client max body size defined in server config
  if (request.is_in_headers("Content-length")) {
    size_t client_max_body_size = request.get_max_body_size();
    std::string str = request.get_value_from_headers("Content-length");
    try {
      body_size = convert_str_to_size(str);
    } catch (const std::exception &e) {
      request.set_status_code(400); // TODO: check
      return false;
    }
    if (body_size > client_max_body_size) {
      request.set_status_code(413);
      return false;
    }
  }
  return true;
}

HttpRequestParser &
HttpRequestParser::operator=(const HttpRequestParser &other) {
  (void)other;
  return *this;
}

/*
3.2.2.  Field Orderより note
A sender MUST NOT generate multiple header fields with the same field
   name in a message unless either the entire field value for that
   header field is defined as a comma-separated list [i.e., #(values)]
   or the header field is a well-known exception (as noted below).

A recipient MAY combine multiple header fields with the same field
   name into one "field-name: field-value" pair, without changing the
   semantics of the message, by appending each subsequent field value to
   the combined field value in order, separated by a comma.  The order
   in which header fields with the same field name are received is
   therefore significant to the interpretation of the combined field
   value; a proxy MUST NOT change the order of these field values when
   forwarding a message.

        Note: In practice, the "Set-Cookie" header field ([RFC6265]) often
      appears multiple times in a response message and does not use the
      list syntax, violating the above requirements on multiple header
      fields with the same name.  Since it cannot be combined into a
      single field-value, recipients ought to handle "Set-Cookie" as a
      special case while processing header fields.  (See Appendix A.2.3
      of [Kri2001] for details.)

      // Cookieには今のところ未対応のつもりだから関係ない　
*/
