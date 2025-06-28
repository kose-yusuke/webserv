#include "CgiParser.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <sstream>

CgiParser::CgiParser()
    : state_(CGI_PARSE_HEADER), status_code_(200), out_buf_(), headers_(), 
    body_(), body_size_(0) {}

CgiParser::~CgiParser() {}

void CgiParser::append(const char *data, size_t length) {
  out_buf_.insert(out_buf_.end(), data, data + length);
}

bool CgiParser::parse(bool eof) {
  LOG_DEBUG_FUNC();
  if (out_buf_.empty() && !(eof && state_ == CGI_PARSE_CHUNK)) {
    return is_done();
  }
  if (state_ == CGI_PARSE_HEADER) {
    parse_headers();
  }
  if (state_ == CGI_PARSE_BODY) {
    parse_body(eof);
  }
  if (state_ == CGI_PARSE_CHUNK) {
    parse_chunked_body(eof);
  }
  if (eof && state_ == CGI_PARSE_HEADER) {
    set_error_status();
  }
  return (is_done() || state_ == CGI_PARSE_CHUNK);
}

bool CgiParser::is_done() const {
  return (state_ == CGI_PARSE_DONE || state_ == CGI_PARSE_ERROR);
}

int CgiParser::get_status_code() const { return status_code_; }

const std::vector<std::pair<std::string, std::string> > &
CgiParser::get_parsed_headers() const {
  return headers_;
}

std::vector<char> CgiParser::take_body() {
  std::vector<char> temp;
  temp.swap(body_); // body_ を空にして temp に入れる
  return temp;
}

static const int k_max_cgi_header_size = 8192;
static const size_t k_max_cgi_body = 1048576;

void CgiParser::parse_headers() {

  LOG_DEBUG_FUNC();

  // ヘッダの終端を探す（\r\n\r\n または \n\n）
  static const char kCRLFCRLF[] = "\r\n\r\n";
  static const char kLFLF[] = "\n\n";

  std::vector<char>::iterator it, it_endpos;
  it = std::search(out_buf_.begin(), out_buf_.end(), kCRLFCRLF, kCRLFCRLF + 4);
  if (it != out_buf_.end()) {
    it_endpos = it + 4;
  } else {
    it = std::search(out_buf_.begin(), out_buf_.end(), kLFLF, kLFLF + 2);
    if (it == out_buf_.end()) {
      if (out_buf_.size() > k_max_cgi_header_size) {
        set_error_status();
      }
      return; // header未受信のため終了
    }
    it_endpos = it + 2;
  }

  // ヘッダ部の終端位置を取得
  size_t header_end = std::distance(out_buf_.begin(), it);
  // ヘッダ文字列を std::string に変換
  std::string header_text(out_buf_.begin(), out_buf_.begin() + header_end);
  // 残りのデータ（＝ボディ）を保持しておくために、ヘッダ部分を削除
  out_buf_.erase(out_buf_.begin(), it_endpos);

  std::istringstream iss(header_text);
  std::string line;

  while (std::getline(iss, line) && !line.empty()) {
    if (!parse_header_line(line)) {
      log(LOG_DEBUG, "Failed to parse CGI header line: " + line);
      set_error_status();
      return;
    }
  }

  if (!is_in_headers("Content-Type")) {
    set_error_status();
    return;
  }

  if (status_code_ == 200 && is_in_headers("Location")) {
    set_status_code(302);
  }

  if (is_in_headers("Content-Length")) {
    std::string body_size_line = get_header("Content-Length");
    std::istringstream ss(body_size_line);
    int tmp_size;

    ss >> tmp_size;
    if (ss.fail() || tmp_size < 0) {
      set_error_status();
      return;
    }

    body_size_ = static_cast<size_t>(tmp_size);
    if (body_size_ > k_max_cgi_body) {
      set_error_status();
      return;
    }

    state_ = CGI_PARSE_BODY;
  } else {
    set_header("Transfer-Encoding", "chunked");
    state_ = CGI_PARSE_CHUNK;
  }
}

static const int k_max_cgi_header_line = 1000;

bool CgiParser::parse_header_line(std::string &line) {

  if (line.size() > k_max_cgi_header_line || std::isspace(line[0])) {
    log(LOG_ERROR, "Invalid cgi header field: " + line);
    return false;
  }

  size_t pos = line.find(":");
  if (pos == std::string::npos || pos == 0) {
    log(LOG_ERROR, "Failed to parse cgi header line: " + line);
    return false;
  }

  std::string key = line.substr(0, pos);    // keyはtrimしない
  std::string value = line.substr(pos + 1); // add_header() でtrim

  for (size_t i = 0; i < key.size(); ++i) {
    if (!is_valid_field_name_char(key[i])) {
      log(LOG_ERROR, "Invalid character in cgi field-name: " + line);
      return false;
    }
  }
  if (to_lower(key) == "status") {
    std::istringstream ss(trim(value));
    int code;
    ss >> code;
    if (!ss.fail()) {
      set_status_code(code);
      return true;
    } else {
      return false; // 呼び出し元でset_error
    }
  }
  set_header(key, value);
  return true;
}

void CgiParser::parse_body(bool eof) {
  if (body_size_ == 0) {
    state_ = CGI_PARSE_DONE;
    return;
  }
  if (out_buf_.size() < body_size_) {
    if (eof) {
      set_error_status();
    }
    return; // body未受信　
  }

  if (out_buf_.size() > body_size_) {
    log(LOG_WARNING, "CGI output exceeds declared Content-Length");
  }
  body_ = std::vector<char>(out_buf_.begin(), out_buf_.begin() + body_size_);
  out_buf_.erase(out_buf_.begin(), out_buf_.begin() + body_size_);
  state_ = CGI_PARSE_DONE;
}

void CgiParser::parse_chunked_body(bool eof) {
  if (out_buf_.empty()) {
    if (eof) {
      state_ = CGI_PARSE_DONE;
    }
    return;
  }

  body_.insert(body_.end(), out_buf_.begin(), out_buf_.end());
  out_buf_.clear(); // 移行済みのbufferを初期化
  if (eof) {
    state_ = CGI_PARSE_DONE;
  }
}

bool CgiParser::is_in_headers(const std::string &key) const {
  for (size_t i = 0; i < headers_.size(); ++i) {
    if (to_lower(headers_[i].first) == to_lower(key)) {
      return true;
    }
  }
  return false;
}

static const char *tchar = "!#$%&'*+-.^_`|~";

bool CgiParser::is_valid_field_name_char(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || std::strchr(tchar, c);
}

std::string CgiParser::get_header(const std::string &key) const {
  for (size_t i = 0; i < headers_.size(); ++i) {
    if (to_lower(headers_[i].first) == to_lower(key)) {
      return headers_[i].second;
    }
  }
  return "";
}

void CgiParser::set_header(const std::string &key, const std::string &value) {
  if (value.empty()) {
    return;
  }
  headers_.push_back(std::pair<std::string, std::string>(key, trim(value)));
}

void CgiParser::set_status_code(int status) {
  if (status_code_ == 500) {
    return;
  }
  status_code_ = status;
}

void CgiParser::set_error_status() {
  state_ = CGI_PARSE_ERROR;
  status_code_ = 500;
}
