#pragma once

#include <map>
#include <string>
#include <vector>

/*
CgiParser:
CGI出力を解析、最小限の検証、ヘッダーとbodyへの分離を行う
*/

class CgiParser {
public:
  // === Constructor & Destructor ===
  CgiParser();
  ~CgiParser();

  // ==== Public API ====
  void append(const char *data, size_t length);
  bool parse(bool eof);
  bool is_done() const;

  int get_status_code() const;
  const std::vector<std::pair<std::string, std::string> > &
  get_parsed_headers() const;
  std::vector<char> take_body();

  size_t body_size_;

private:
  enum CgiParseState {
    CGI_PARSE_HEADER, // header 読み込み中
    CGI_PARSE_BODY,   // body 読み込み中
    CGI_PARSE_CHUNK,  // chunked body 読み込み中
    CGI_PARSE_ERROR,  // 解析のerror終了
    CGI_PARSE_DONE    // 解析の正常終了
  };

  // パース状態の管理とレスポンスのstatus
  CgiParseState state_;
  int status_code_;

  // CGIの未処理出力buffer
  std::vector<char> out_buf_;

  // parsed data (ヘッダとbodyに分離)
  std::vector<std::pair<std::string, std::string> > headers_;
  std::vector<char> body_;

  // ==== Main parse logic ====
  void parse_headers();
  bool parse_header_line(std::string &line);
  void parse_body(bool eof);
  void parse_chunked_body(bool eof);

  // ==== Header utilities ====
  bool is_in_headers(const std::string &key) const;
  std::string get_header(const std::string &key) const;
  void set_header(const std::string &key, const std::string &value);

  // ==== Other ====
  bool is_valid_field_name_char(char c);
  void set_error_status();
  void set_status_code(int status);
};
