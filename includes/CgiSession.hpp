#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

class HttpRequest;
class HttpResponse;

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
CgiSession: CGIの起動とpipe通信の制御-
*/
class CgiSession {
public:
  enum CgiState { WAIT_WRITE_BODY, WAIT_READ_OUTPUT, DONE, ERROR, TIMED_OUT };

  // Constructor / Destructor / Assignment
  CgiSession(HttpRequest &request, HttpResponse &response, int client_fd);
  ~CgiSession();

  // Accessors
  int get_client_fd() const;
  int get_stdin_fd() const;
  int get_stdout_fd() const;

  // State checkers
  bool is_cgi_active() const;
  bool is_cgi_timeout(time_t now) const;

  // Main processing logic
  void handle_cgi_request(const std::string &cgi_path,
                          std::vector<char> body_data, std::string method,
                          std::string path);

  // event 発火
  CgiIOStatus on_cgi_write(); // write body to CGI
  CgiIOStatus on_cgi_read();  // read output from CGI
  void on_cgi_timeout();
  void on_client_timeout();
  void on_client_abort();

private:
  HttpRequest &request_;
  HttpResponse &response_;

  const int client_fd_;
  CgiState state_;
  pid_t pid_;
  int stdin_fd_;
  int stdout_fd_;

  ConnectionPolicy conn_policy_;
  std::vector<char> body_buf_;
  size_t write_offset_;
  bool is_header_sent_;

  time_t cgi_last_activity_;

  // Helpers
  void handle_cgi_header();
  void handle_cgi_done();
  void handle_cgi_error();
  void terminate_pid();
  void terminate_cgi_fds();
  bool is_terminal_state() const;

  void reset();
  void update_cgi_activity();

  CgiSession(const CgiSession &other); // copy 禁止
  CgiSession &operator=(const CgiSession &other);
};
