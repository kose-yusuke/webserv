#include "HttpRequestParser.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <set>

static const char *unreserved_chars = "-._~";
static const char *reserved_chars = ":/?#[]@!$&'()*+,;=";

const size_t HttpRequestParser::k_max_request_line = 10000;
const size_t HttpRequestParser::k_max_request_target = 2048;

static const char *methods_arr[] = {"GET",    "HEAD",    "POST",    "PUT",
                                    "DELETE", "CONNECT", "OPTIONS", "TRACE"};

static const std::set<std::string> supported_methods(
    methods_arr, methods_arr + sizeof(methods_arr) / sizeof(methods_arr[0]));

HttpRequestParser::HttpRequestParser(HttpRequest &http_request)
    : request(http_request), parse_state(PARSE_HEADER) {}

HttpRequestParser::~HttpRequestParser() {}

// TODO: non-framing errorの時に、bodyを読み終えてから parse を終了する
bool HttpRequestParser::parse() {
  LOG_DEBUG_FUNC();
  if (recv_buffer.empty()) {
    return parse_state;
  }
  if (parse_state == PARSE_HEADER) {
    parse_state = parse_header();
  }
  if (parse_state == PARSE_BODY) {
    parse_state = parse_body();
  }
  if (parse_state == PARSE_CHUNK) {
    parse_state = parse_chunked_body();
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
  recv_buffer.insert(recv_buffer.end(), data, data + length);
}

HttpRequestParser::ParseState HttpRequestParser::parse_header() {
  LOG_DEBUG_FUNC();
  static const char kCRLFCRLF[] = "\r\n\r\n";

  std::vector<char>::iterator it = std::search(
      recv_buffer.begin(), recv_buffer.end(), kCRLFCRLF, kCRLFCRLF + 4);
  if (it == recv_buffer.end()) {
    if (recv_buffer.size() >= k_max_request_line) {
      request.set_status_code(431); // Header Fields Too Large
      return PARSE_ERROR;
    }
    return PARSE_HEADER;
  }

  size_t header_end = std::distance(recv_buffer.begin(), it);
  std::string header_text(recv_buffer.begin(),
                          recv_buffer.begin() + header_end);

  std::istringstream iss(header_text);
  std::string line;

  recv_buffer.erase(recv_buffer.begin(), it + 4); // "\r\n\r\n"まで削除
  if (!std::getline(iss, line) || !parse_request_line(line)) {
    request.set_status_code(400);
    request.set_connection_policy(CP_MUST_CLOSE);
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
  if (request.method == "POST" && !validate_headers_content()) {
    return PARSE_ERROR;
  }
  log(LOG_DEBUG, "Parse Success !!");
  determine_connection_policy();
  return next_parse_state();
}

HttpRequestParser::ParseState HttpRequestParser::next_parse_state() const {
  if (request.method == "GET" || request.method == "DELETE") {
    return PARSE_DONE;
  }
  if (request.is_in_headers("Transfer-Encoding")) {
    return PARSE_CHUNK;
  }
  if (request.is_in_headers("Content-Length")) {
    return PARSE_BODY;
  }
  return PARSE_DONE;
}

HttpRequestParser::ParseState HttpRequestParser::parse_body() {
  LOG_DEBUG_FUNC();
  // TODO: timeout 処理
  if (body_size == 0) {
    return PARSE_DONE; // bodyなし
  }
  if (body_size > 0 && recv_buffer.size() < body_size) {
    return PARSE_BODY; // body未受信
  }
  request.body_data.insert(request.body_data.end(), recv_buffer.begin(),
                           recv_buffer.begin() + body_size);
  recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + body_size);
  // request.body.append(buffer.substr(0, body_size));
  // buffer.erase(0, body_size);
  return PARSE_DONE; // body受信完了
}

HttpRequestParser::ParseState HttpRequestParser::parse_chunked_body() {
  LOG_DEBUG_FUNC();
  // TODO: chunked終端が届かない時の処理
  size_t chunk_size = 0;
  static const char kCRLF[] = "\r\n";

  while (true) {
    std::vector<char>::iterator it_size =
        std::search(recv_buffer.begin(), recv_buffer.end(), kCRLF, kCRLF + 2);
    if (it_size == recv_buffer.end()) {
      return PARSE_CHUNK;
    }
    size_t end_pos = std::distance(recv_buffer.begin(), it_size);
    std::string size_str(recv_buffer.begin(), recv_buffer.begin() + end_pos);

    // size_t pos = buffer.find("\r\n");
    // if (pos == std::string::npos) {
    //   return PARSE_CHUNK;
    // }
    // std::string size_str = buffer.substr(0, pos);
    try {
      chunk_size = parse_hex(size_str);
    } catch (const std::exception &e) {
      request.set_status_code(400);
      return PARSE_ERROR;
    }
    if (chunk_size == 0) {
      break;
    }

    size_t chunk_start = end_pos + 2;
    size_t chunk_end = chunk_start + chunk_size + 2;
    if (recv_buffer.size() < chunk_end) {
      return PARSE_CHUNK;
    }

    std::vector<char> chunk(recv_buffer.begin() + chunk_start,
                            recv_buffer.begin() + chunk_end);
    request.body_data.insert(request.body_data.end(), chunk.begin(),
                             chunk.end());
    recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + chunk_end);
    // std::string chunk = buffer.substr(chunk_start, chunk_size);
    // request.body.append(chunk);
    // buffer.erase(0, chunk_end);
  }

  static const char kFinalChunk[] = "0\r\n\r\n";

  if (recv_buffer.size() < 5) {
    return PARSE_CHUNK;
  }

  if (!std::equal(kFinalChunk, kFinalChunk + 5, recv_buffer.begin())) {
    request.set_status_code(400);
    return PARSE_ERROR;
  }
  recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + 5);

  // if (buffer.substr(0, 5) != "0\r\n\r\n") {
  //   request.set_status_code(400);
  //   return PARSE_ERROR;
  // }
  // buffer.erase(0, 5);
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
  // if (!request.add_header(key, value)) {
  //   log(LOG_ERROR, "Duplicate header found: " + key);
  //   // TODO: issue #59 あとで
  //   // TODO: exceptionあるので注意 ex. CSV, or known exception
  //   return false;
  // }
  // TODO: singleton fields, list-based fields の確認
  request.add_header(key, value);

  log(LOG_DEBUG, "Parsed - Key: " + key + ": " + request.get_header_value(key));
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

  for (size_t i = 0; i < request.path.size(); ++i) {
    char c = request.path.at(i);
    if (!std::isdigit(c) && !std::isalpha(c) &&
        !std::strchr(unreserved_chars, c) && !std::strchr(reserved_chars, c)) {
      log(LOG_DEBUG, std::string("Invalid character in target: '") + c + "'");
      request.set_status_code(400);
      return false;
    }
  }

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

  // Transfer-Encoding と Content-Length の併存
  if (request.is_in_headers("Transfer-Encoding") &&
      request.is_in_headers("Content-Length")) {
    request.set_status_code(400);
    request.set_connection_policy(CP_MUST_CLOSE);
    return false;
  }

  // HTTP/1.0 かつ Transfer-Encoding ヘッダーあり
  // TODO: 現在 HTTP/1.0 対応はないが 後方互換性が必要になる可能性を考慮して記述
  if (request.version == "HTTP/1.0" &&
      request.is_in_headers("Transfer-Encoding")) {
    request.set_status_code(400);
    request.set_connection_policy(CP_MUST_CLOSE);
    return false;
  }

  // Transfer-Encoding  あり. but not chunked
  if (request.is_in_headers("Transfer-Encoding") &&
      request.get_header_value("Transfer-Encoding") != "chunked") {
    request.set_status_code(501);
    return false;
  }

  // Transfer-Encoding も Content-length もない POST
  if (!request.is_in_headers("Transfer-Encoding") &&
      !request.is_in_headers("Content-Length")) {
    request.set_status_code(400);
    return false;
  }

  // Request body larger than client max body size defined in server config
  if (request.is_in_headers("Content-Length")) {
    size_t client_max_body_size = request.get_max_body_size();
    std::string str = request.get_header_value("Content-Length");
    try {
      body_size = str_to_size(str);
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

void HttpRequestParser::determine_connection_policy() {
  if (request.get_connection_policy() == CP_MUST_CLOSE) {
    // parse中に、framing errorを見つけてある
    return;
  }

  const std::string conn_header = request.get_header_value("Connection");
  if (conn_header == "close") {
    request.set_connection_policy(CP_WILL_CLOSE);
  } else if (request.version == "HTTP/1.1") {
    request.set_connection_policy(CP_KEEP_ALIVE);
  } else if (request.version == "HTTP/1.0" && conn_header == "keep-alive") {
    request.set_connection_policy(CP_KEEP_ALIVE);
  } else {
    request.set_connection_policy(CP_WILL_CLOSE);
  }
}

HttpRequestParser &
HttpRequestParser::operator=(const HttpRequestParser &other) {
  (void)other;
  return *this;
}
