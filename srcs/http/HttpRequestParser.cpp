#include "HttpRequestParser.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <set>

static const char *unreserved_chars = "-._~";
static const char *reserved_chars = ":/?#[]@!$&'()*+,;=";
static const char *tchar = "!#$%&'*+-.^_`|~";

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
  if (recv_buffer.empty()) {
    return (parse_state == PARSE_DONE);
  }
  if (parse_state == PARSE_HEADER) {
    parse_header();
  }
  if (parse_state == PARSE_BODY) {
    parse_body();
  }
  if (parse_state == PARSE_CHUNK) {
    parse_chunked_body();
  }
  return (parse_state == PARSE_DONE);
}

void HttpRequestParser::clear() {
  LOG_DEBUG_FUNC();
  request.clear();
  parse_state = PARSE_HEADER;
  body_size = 0;
}

void HttpRequestParser::append_data(const char *data, size_t length) {
  LOG_DEBUG_FUNC();
  recv_buffer.insert(recv_buffer.end(), data, data + length);
}

void HttpRequestParser::parse_header() {
  LOG_DEBUG_FUNC();
  static const char kCRLFCRLF[] = "\r\n\r\n";

  std::vector<char>::iterator it = std::search(
      recv_buffer.begin(), recv_buffer.end(), kCRLFCRLF, kCRLFCRLF + 4);
  if (it == recv_buffer.end()) {
    if (recv_buffer.size() >= k_max_request_line) {
      request.set_status_code(431); // Header Fields Too Large
    }
    return;
  }

  size_t header_end = std::distance(recv_buffer.begin(), it);
  std::string header_text(recv_buffer.begin(),
                          recv_buffer.begin() + header_end);

  std::istringstream iss(header_text);
  std::string line;

  recv_buffer.erase(recv_buffer.begin(), it + 4); // "\r\n\r\n"まで削除
  if (!std::getline(iss, line) || !parse_request_line(line)) {
    log(LOG_DEBUG, "Failed to parse request line");
    set_framing_error(400);
    return;
  }
  validate_request_content();

  while (std::getline(iss, line) && !line.empty()) {
    if (!parse_header_line(line)) {
      log(LOG_DEBUG, "Failed to parse header line: " + line);
      set_framing_error(400);
      return;
    }
  }
  if (!check_framing_error()) {
    log(LOG_DEBUG, "Framing error detected by checking");
    set_framing_error(400);
    return;
  }
  validate_headers_content();
  determine_connection_policy();
  next_parse_state();
}

void HttpRequestParser::next_parse_state() {
  LOG_DEBUG_FUNC();
  if (request.is_in_headers("Content-Length")) {
    parse_state = PARSE_BODY;
  } else if (request.is_in_headers("Transfer-Encoding")) {
    parse_state = PARSE_CHUNK;
  } else {
    parse_state = PARSE_DONE;
  }
}

void HttpRequestParser::parse_body() {
  LOG_DEBUG_FUNC();
  if (body_size == 0) {
    parse_state = PARSE_DONE;
    return;
  }
  if (recv_buffer.size() < body_size) {
    return; // body未受信
  }
  request.body_data_.insert(request.body_data_.end(), recv_buffer.begin(),
                            recv_buffer.begin() + body_size);
  recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + body_size);
  parse_state = PARSE_DONE; // body受信完了
}

void HttpRequestParser::parse_chunked_body() {
  LOG_DEBUG_FUNC();
  size_t chunk_size = 0;
  static const char kCRLF[] = "\r\n";

  while (true) {
    std::vector<char>::iterator it_size =
        std::search(recv_buffer.begin(), recv_buffer.end(), kCRLF, kCRLF + 2);
    if (it_size == recv_buffer.end()) {
      return; // size 未取得
    }
    std::string size_str(recv_buffer.begin(), it_size);
    size_str = size_str.substr(0, size_str.find(';'));

    try {
      chunk_size = parse_hex(size_str);
    } catch (const std::exception &e) {
      log(LOG_ERROR, "Failed to parse chunk size: " + size_str);
      set_framing_error(400);
      return;
    }

    if (chunk_size == 0) {
      break; // chunked body 終端
    }

    size_t data_start = std::distance(recv_buffer.begin(), it_size) + 2;
    size_t data_end = data_start + chunk_size;
    if (recv_buffer.size() < data_end + 2) {
      return; // size 分の chunk 未取得
    }

    std::vector<char> chunk(recv_buffer.begin() + data_start,
                            recv_buffer.begin() + data_end);
    request.body_data_.insert(request.body_data_.end(), chunk.begin(),
                              chunk.end());
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + data_end + 2);
  }

  if (recv_buffer.size() < 5 ||
      std::memcmp(&recv_buffer[0], "0\r\n\r\n", 5) != 0) {
    set_framing_error(400);
    return;
  }
  recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 5);
  parse_state = PARSE_DONE;
}

bool HttpRequestParser::parse_request_line(std::string &line) {
  LOG_DEBUG_FUNC();

  if (line.empty() || line.size() > k_max_request_line ||
      std::isspace(line[0])) {
    return false;
  }

  std::istringstream request_iss(line);
  if (!(request_iss >> request.method_ >> request.path_ >> request.version_)) {
    log(LOG_ERROR, "Failed to parse request line: " + line);
    return false;
  }

  std::string ext;
  if (request_iss >> ext) {
    log(LOG_DEBUG, "Extra characters (" + ext + ") in request line");
    return false;
  }

  if (request.method_.empty() || request.path_.empty() ||
      request.version_.empty()) {
    log(LOG_DEBUG, "Failed to parse request line (empty value)");
    return false;
  }

  for (size_t i = 0; i < request.path_.size(); ++i) {
    char c = request.path_.at(i);
    if (!std::isdigit(c) && !std::isalpha(c) &&
        !std::strchr(unreserved_chars, c) && !std::strchr(reserved_chars, c)) {
      log(LOG_DEBUG, std::string("Invalid character in target: '") + c + "'");
      return false;
    }
  }

  log(LOG_DEBUG, "Parsed Request - Method: " + request.method_ + ", Path: " +
                     request.path_ + ", Version: " + request.version_);
  return true;
}

bool HttpRequestParser::parse_header_line(std::string &line) {

  if (line.size() > k_max_request_line || std::isspace(line[0])) {
    log(LOG_ERROR, "Invalid header field: " + line);
    return false;
  }

  size_t pos = line.find(":");
  if (pos == std::string::npos || pos == 0) {
    log(LOG_ERROR, "Failed to parse header line: " + line);
    return false;
  }

  std::string key = line.substr(0, pos);    // keyはtrimしない
  std::string value = line.substr(pos + 1); // add_header() でtrim

  for (size_t i = 0; i < key.size(); ++i) {
    if (!is_valid_field_name_char(key[i])) {
      log(LOG_ERROR, "Invalid character in field-name: " + line);
      return false;
    }
  }
  request.add_header(key, value);

  log(LOG_DEBUG, "Parsed - Key: " + key + ": " + request.get_header_value(key));
  return true;
}

void HttpRequestParser::validate_request_content() {
  LOG_DEBUG_FUNC();

  if (request.get_status_code() != 0) {
    return;
  }

  // path が長すぎる
  if (request.path_.size() > k_max_request_target) {
    log(LOG_DEBUG, "Request-target is too long");
    request.set_status_code(414);
    return;
  }

  // method 不正
  if (supported_methods.find(request.method_) == supported_methods.end()) {
    log(LOG_DEBUG, "Unsupported HTTP method: " + request.method_);
    request.set_status_code(501);
    return;
  }

  // HTTP version 不正
  if (request.version_ != "HTTP/1.1") {
    log(LOG_ERROR, "Unsupported HTTP version: " + request.version_);
    request.set_status_code(505);
  }
}

bool HttpRequestParser::check_framing_error() {
  LOG_DEBUG_FUNC();

  // Hostヘッダが存在しない or 重複する
  if (!request.is_in_headers("Host") ||
      request.get_header_value("Host").empty() ||
      request.get_header_values("Host").size() > 1) {
    log(LOG_ERROR, "Invalid Host header");
    return false;
  }

  // Transfer-Encoding と Content-Length の併存
  if (request.is_in_headers("Transfer-Encoding") &&
      request.is_in_headers("Content-Length")) {
    return false;
  }

  // HTTP/1.0 かつ Transfer-Encoding ヘッダーあり
  // 現在 HTTP/1.0 対応はないが 後方互換性が必要になる可能性を考慮して記述
  if (request.version_ == "HTTP/1.0" &&
      request.is_in_headers("Transfer-Encoding")) {
    return false;
  }

  // Transfer-Encoding あり chunked が最後のエンコーディングでない
  // header valueには空の値（""）はないものとする
  if (request.is_in_headers("Transfer-Encoding")) {
    StrVector values = request.get_header_values("Transfer-Encoding");
    if (values.empty() || values.back() != "chunked") {
      return false;
    }
  }

  // Content-Length のに指定される値が不正または異なる値が複数指定される
  if (request.is_in_headers("Content-Length")) {

    StrVector num_values = request.get_header_values("Content-Length");
    if (num_values.empty()) {
      return false;
    }
    try {
      body_size = str_to_size(num_values[0]);
    } catch (const std::exception &e) {
      return false;
    }
    for (size_t i = 1; i < num_values.size(); ++i) {
      if (num_values[i] != num_values[0]) {
        return false;
      }
    }
  }
  return true;
}

void HttpRequestParser::validate_headers_content() {
  LOG_DEBUG_FUNC();

  // Transfer-Encoding も Content-length もない POST
  if (request.method_ == "POST" &&
      !request.is_in_headers("Transfer-Encoding") &&
      !request.is_in_headers("Content-Length")) {
    request.set_status_code(400);
  }
}

void HttpRequestParser::determine_connection_policy() {
  if (request.get_connection_policy() == CP_MUST_CLOSE) {
    return;
  }
  const std::string conn_header = request.get_header_value("Connection");
  if (conn_header == "close") {
    request.set_connection_policy(CP_WILL_CLOSE);
  } else if (request.version_ == "HTTP/1.1") {
    request.set_connection_policy(CP_KEEP_ALIVE);
  } else if (request.version_ == "HTTP/1.0" && conn_header == "keep-alive") {
    request.set_connection_policy(CP_KEEP_ALIVE);
  } else {
    request.set_connection_policy(CP_WILL_CLOSE);
  }
}

bool HttpRequestParser::is_valid_field_name_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || std::strchr(tchar, c);
}

void HttpRequestParser::set_framing_error(int status) {
  LOG_DEBUG_FUNC();
  request.set_status_code(status);
  request.set_connection_policy(CP_MUST_CLOSE);
  parse_state = PARSE_DONE;
}

HttpRequestParser &
HttpRequestParser::operator=(const HttpRequestParser &other) {
  (void)other;
  return *this;
}
