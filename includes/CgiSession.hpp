#pragma once

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "types.hpp"
#include <ctime>
#include <errno.h>
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

class CgiSession {
public:
  enum CgiState {
    WAIT_WRITE_BODY,
    WAIT_READ_OUTPUT,
    WAIT_CHILD_EXIT,
    DONE,
    ERROR
  };

  CgiSession(HttpRequest &request, HttpResponse &response, int client_fd);
  ~CgiSession();

  int get_stdin_fd() const;
  int get_stdout_fd() const;
  int get_client_fd() const;
  bool is_done() const;
  bool is_timeout(time_t now) const;

  void handle_cgi_request(const std::string &cgi_path,
                          std::vector<char> body_data, std::string method,
                          std::string path);
  CgiIOStatus on_write(); // write body to CGI
  CgiIOStatus on_read();  // read output from CGI
  bool on_done();
  void on_timeout();

private:
  HttpRequest &request_;
  HttpResponse &response_;

  pid_t pid_;
  int stdin_fd_;
  int stdout_fd_;
  int client_fd_;
  CgiState state_;

  std::vector<char> body_buf_;
  size_t write_offset_;
  std::string output_buf_;

  time_t cgi_timeout_sec_;
  time_t cgi_start_time_;
  time_t cgi_last_activity_;

  void set_cgi_start_time();
  void update_cgi_activity();

  CgiSession(const CgiSession &other);
  CgiSession &operator=(const CgiSession &other);
};
