#pragma once

#include "CgiParser.hpp"
#include "CgiResponseBuilder.hpp"
#include "HttpRequest.hpp"
#include "Logger.hpp"
#include "ResponseTypes.hpp"
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

enum CgiIOStatus {

  CGI_IO_CONTINUE,       // 現状維持（read/write継続）
  CGI_IO_READY_TO_WRITE, // write監視をON
  CGI_IO_WRITE_COMPLETE, // write監視をOFF
  CGI_IO_READ_COMPLETE,
  CGI_IO_SHOULD_SHUTDOWN, // shutdown(fd, SHUT_WR)
  CGI_IO_SHOULD_CLOSE,    // close(fd)
  CGI_IO_ERROR
};

/*
方針
- Client -> CGI の一方向参照
- 通常時は、ClientがCGI sessionを片付ける
- Clientが先に死んでいる場合のみ、自主的にCGI sessionを終了する
- CGI session はbufにCGI IOの出力を貯めるだけで, parseやレスポンス生成を行わない
*/

/*
CgiSession: CGIの起動とpipe通信の制御
*/
class CgiSession {
public:
  // enum 再検討
  enum CgiState {
    CGI_IDLE,
    CGI_WRITING,
    CGI_READING,
    CGI_EOF,
    CGI_ERROR,
    CGI_TIMED_OUT
  };

  // Constructor / Destructor / Assignment
  CgiSession(int client_fd);
  ~CgiSession();

  // Accessors
  int get_client_fd() const { return client_fd_; };
  int get_stdin_fd() const;
  int get_stdout_fd() const;
  void mark_client_dead();

  // State checkers
  bool is_client_alive() const;
  bool is_processing() const;
  bool is_failed() const;
  bool is_eof() const;
  bool is_session_completed() const;
  bool is_cgi_timeout(time_t now) const;

  // Main processing logic
  void handle_cgi_request(HttpRequest &request, const std::string &cgi_path);
  void build_response(HttpResponse &response);
  void build_error_response(HttpResponse &response, int status_code,
                            ConnectionPolicy policy);

  void close_fd(int fd);

  // event 発火
  CgiIOStatus on_cgi_write(); // write body to CGI
  CgiIOStatus on_cgi_read();  // read output from CGI
  void on_cgi_timeout();

private:
  CgiParser parser_;
  CgiResponseBuilder builder_;

  int client_fd_;
  CgiState state_; // 実行状態: WAIT_WRITE, WAIT_READ, DONE, ERROR...
  pid_t pid_;      // 子プロセスPID
  int stdin_fd_;   // CGI入力パイプ (親 -> 子)
  int stdout_fd_;  // CGI出力パイプ (子 -> 親)

  std::vector<char> in_buf_; // 入力データ
  size_t in_off_;            // 入力進捗のoffset

  time_t cgi_last_activity_; // timeout監視用タイムスタンプ
  bool client_alive_; // Clientが死んだ時に、CgiSessionのself clean upを喚起

  // Helpers
  void terminate_pid();
  void terminate_cgi_fds();
  bool is_terminal_state() const;

  void update_cgi_activity();

  CgiSession(const CgiSession &other); // copy 禁止
  CgiSession &operator=(const CgiSession &other);
};
