#include "HttpRequestParser.hpp"
#include "Utils.hpp"
#include <set>

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
}

void HttpRequestParser::append_data(const char *data, size_t length) {
  LOG_DEBUG_FUNC();

  buffer.append(data, length);
}

HttpRequestParser::ParseState HttpRequestParser::parse_body() {
  LOG_DEBUG_FUNC();

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
    // log(LOG_DEBUG, "Parsing line: " + line);
    if (!parse_header_line(line)) {
      request.set_status_code(400);
      return PARSE_ERROR;
    }
  }
  if (!validate_headers_content()) {
    return PARSE_ERROR;
  }
  log(LOG_DEBUG, "Parse Success !!");
  // TODO: parse-body() にあたるかのチェック
  // TODO: Transfer-Encoding: chunked 対応
  // ex. parse_state == PARSE_CHUNKED
  return PARSE_DONE;
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

  if (request.path.size() > MAX_REQUEST_TARGET) {
    log(LOG_DEBUG, "Request-target is too long");
    request.set_status_code(414);
    return false;
  }

  // TODO: Request url が NG文字を含む
  // request.set_status_code(400);

  if (request.version != "HTTP/1.1") {
    log(LOG_ERROR, "Unsupported HTTP version: " + request.version);
    request.set_status_code(505);
    return false;
  }

  return true;
}

bool HttpRequestParser::validate_headers_content() {
  LOG_DEBUG_FUNC();

  // TODO: Transfer-Encoding あり. but not chunked
  // request.set_status_code(501);

  // Transfer-Encoding も Content-length もある POST
  // request.set_status_code(400);

  // Request body larger than client max body size in config
  // request.set_status_code(413);
  return true;
}

bool HttpRequestParser::parse_request_line(std::string &line) {
  LOG_DEBUG_FUNC();

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
  LOG_DEBUG_FUNC();
  if (line.size() > MAX_REQUEST_LINE || std::isspace(line[0])) {
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

/*
// TODO
 The backslash octet ("\") can be used as a single-octet quoting
   mechanism within quoted-string and comment constructs.  Recipients
   that process the value of a quoted-string MUST handle a quoted-pair
   as if it were replaced by the octet following the backslash.

     quoted-pair    = "\" ( HTAB / SP / VCHAR / obs-text )


*/
