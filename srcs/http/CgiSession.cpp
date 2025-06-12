#include "CgiSession.hpp"
#include "Logger.hpp"
#include "Multiplexer.hpp"
#include <iostream>
#include <signal.h>

static const int k_cgi_timeout_sec = 10;

CgiSession::CgiSession(HttpRequest &request, HttpResponse &response, int fd)
    : request_(request), response_(response), client_fd_(fd), state_(DONE),
      pid_(-1), stdin_fd_(-1), stdout_fd_(-1), conn_policy_(CP_KEEP_ALIVE),
      is_header_sent_(false), cgi_last_activity_(time(NULL)) {}

CgiSession::~CgiSession() {}

int CgiSession::get_client_fd() const { return client_fd_; }

int CgiSession::get_stdin_fd() const { return stdin_fd_; }

int CgiSession::get_stdout_fd() const { return stdout_fd_; }

bool CgiSession::is_cgi_active() const {
  return (state_ == WAIT_WRITE_BODY || state_ == WAIT_READ_OUTPUT);
}

bool CgiSession::is_cgi_timeout(time_t now) const {
  if (state_ == DONE || state_ == ERROR) {
    return false;
  }
  return (state_ == TIMED_OUT || now - cgi_last_activity_ > k_cgi_timeout_sec);
}

void CgiSession::handle_cgi_request(const std::string &cgi_path,
                                    std::vector<char> body_data,
                                    std::string method, std::string path) {
  LOG_DEBUG_FUNC();
  int input_pipe[2];
  int output_pipe[2];

  if (pipe(input_pipe) == -1) {
    log(LOG_ERROR, "pipe (stdin) failed"); // この中でstrerrorを使用
    std::exit(1);
  }
  if (pipe(output_pipe) == -1) {
    close(input_pipe[0]);
    close(input_pipe[1]);
    log(LOG_ERROR, "pipe (stdout) failed");
    std::exit(1);
  }

  body_buf_ = body_data;
  write_offset_ = 0;

  pid_ = fork();
  if (pid_ < 0) {
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    log(LOG_ERROR, "fork failed");
    std::exit(1);
  }

  if (pid_ == 0) {

    // 標準入出力のリダイレクト
    dup2(input_pipe[0], STDIN_FILENO);
    dup2(output_pipe[1], STDOUT_FILENO);

    // 不要なfdを全て閉じる
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);

    // 環境変数の設定
    std::string contentLength = request_.get_header_value("Content-Length");
    std::string contentLengthStr = "CONTENT_LENGTH=" + contentLength;
    std::string requestMethodStr = "REQUEST_METHOD=POST";
    std::string contentTypeStr =
        "CONTENT_TYPE=" + request_.get_header_value("Content-Type");
    std::string queryString = "QUERY_STRING=";

    // TODO: 確認: QUERY_STRING += body;でPOSTがbodyを再び渡している
    if (method == "POST") {
      std::string body(body_data.begin(), body_data.end());
      queryString += body;
    } else if (method == "GET") {
      size_t pos = path.find('?');
      if (pos != std::string::npos) {
        queryString += path.substr(pos + 1);
      }
    }
    (void)cgi_path;

    char *envp[] = {const_cast<char *>(requestMethodStr.c_str()),
                    const_cast<char *>(contentLengthStr.c_str()),
                    const_cast<char *>(contentTypeStr.c_str()),
                    const_cast<char *>(queryString.c_str()), NULL};

    char *argv[] = {const_cast<char *>(cgi_path.c_str()), NULL};

    execve(cgi_path.c_str(), argv, envp);
    log(LOG_ERROR, "execve failed");
    std::exit(1);
  } else {
    // 親プロセスで使わないpipeを閉じる
    close(input_pipe[0]);
    close(output_pipe[1]);

    // Multiplexerに登録するfdを設定
    stdin_fd_ = input_pipe[1];
    stdout_fd_ = output_pipe[0];

    // pipeのfd（親側）をnon blockingにする
    if (fcntl(stdin_fd_, F_SETFL, fcntl(stdin_fd_, F_GETFL) | O_NONBLOCK) ==
            -1 ||
        fcntl(stdout_fd_, F_SETFL, fcntl(stdout_fd_, F_GETFL) | O_NONBLOCK) ==
            -1) {
      log(LOG_ERROR, "fcntl to cgi fd failed");
      close(stdin_fd_);
      close(stdout_fd_);
      std::exit(1);
    }

    // ClientRegistryに登録し、stdin_fd_の監視を開始する
    Multiplexer &multiplexer = Multiplexer::get_instance();
    multiplexer.register_cgi_fds(stdin_fd_, stdout_fd_, this);
    multiplexer.monitor_write(stdin_fd_);
    state_ = WAIT_WRITE_BODY;
    conn_policy_ = request_.get_connection_policy();
  }
}

CgiIOStatus CgiSession::on_cgi_write() {
  LOG_DEBUG_FUNC();
  if (!is_cgi_active()) {
    return CGI_IO_ERROR;
  }
  if (body_buf_.empty()) {
    state_ = WAIT_READ_OUTPUT;
    return CGI_IO_WRITE_COMPLETE;
  }
  ssize_t bytes_write = write(stdin_fd_, &body_buf_[0] + write_offset_,
                              body_buf_.size() - write_offset_);
  if (bytes_write == -1) {
    handle_cgi_error();
    return CGI_IO_ERROR;
  }
  update_cgi_activity();
  write_offset_ += bytes_write;
  if (write_offset_ < body_buf_.size()) {
    return CGI_IO_CONTINUE;
  }
  body_buf_.clear();
  write_offset_ = 0;
  state_ = WAIT_READ_OUTPUT;
  return CGI_IO_WRITE_COMPLETE;
}

CgiIOStatus CgiSession::on_cgi_read() {
  LOG_DEBUG_FUNC();
  if (!is_cgi_active()) {
    return CGI_IO_ERROR;
  }
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = read(stdout_fd_, buffer, buf_size);
  if (bytes_read == -1) {
    std::cout << "bytes_read == -1\n";
    handle_cgi_error();
    return CGI_IO_ERROR;
  }
  update_cgi_activity();
  handle_cgi_header();
  if (!is_header_sent_) {
    handle_cgi_error();
    return CGI_IO_ERROR;
  }
  if (bytes_read == 0) {
    std::cout << "bytes_read == 0\n";
    handle_cgi_done();
    return CGI_IO_READ_COMPLETE;
  }
  std::string chunk(buffer, bytes_read);
  response_.generate_chunk_response_body(chunk, conn_policy_);
  return CGI_IO_CONTINUE;
}

void CgiSession::on_cgi_timeout() {
  if (is_terminal_state()) {
    return;
  }
  LOG_DEBUG_FUNC();
  state_ = TIMED_OUT;
  terminate_pid();
  if (is_header_sent_) {
    response_.generate_chunk_response_last(conn_policy_);
  } else {
    response_.generate_error_response(500, conn_policy_);
  }
}

void CgiSession::on_client_timeout() {
  if (!is_cgi_active()) {
    return; // すでにcgi終了済み
  }
  LOG_DEBUG_FUNC();
  if (is_header_sent_) {
    response_.generate_chunk_response_last(conn_policy_);
  } else {
    response_.generate_timeout_response();
  }
  terminate_pid();
  terminate_cgi_fds();
  state_ = TIMED_OUT;
}

void CgiSession::on_client_abort() {
  if (!is_cgi_active()) {
    return; // すでにcgi終了済み
  }
  LOG_DEBUG_FUNC();
  terminate_pid();
  terminate_cgi_fds();
  state_ = ERROR;
}

void CgiSession::handle_cgi_header() {
  if (is_header_sent_) {
    return;
  }
  LOG_DEBUG_FUNC();
  int status;
  if (waitpid(pid_, &status, WNOHANG) != 0 &&
      (WIFEXITED(status) && WEXITSTATUS(status) != 0)) {
    pid_ = -1; // track_zombie などが誤って呼ばれないように
    return;
  }
  response_.generate_chunk_response_header(conn_policy_);
  is_header_sent_ = true;
}

void CgiSession::handle_cgi_done() {
  if (!is_cgi_active() || pid_ == -1) {
    return;
  }
  LOG_DEBUG_FUNC();
  state_ = DONE;
  Multiplexer::get_instance().track_zombie(pid_);
  pid_ = -1;
  terminate_cgi_fds();
  response_.generate_chunk_response_last(conn_policy_);
}

void CgiSession::handle_cgi_error() {
  if (is_terminal_state()) {
    return;
  }
  LOG_DEBUG_FUNC();
  state_ = ERROR;
  terminate_pid();
  if (is_header_sent_) {
    response_.generate_chunk_response_last(conn_policy_);
  } else {
    response_.generate_error_response(500, conn_policy_);
  }
}

void CgiSession::terminate_pid() {
  if (pid_ == -1) {
    return;
  }
  LOG_DEBUG_FUNC();
  kill(pid_, SIGTERM);
  Multiplexer::get_instance().track_zombie(pid_);
  pid_ = -1;
}

bool CgiSession::is_terminal_state() const {
  return (state_ == ERROR || state_ == TIMED_OUT);
}

void CgiSession::terminate_cgi_fds() {
  LOG_DEBUG_FUNC();
  Multiplexer &multiplexer = Multiplexer::get_instance();
  if (stdin_fd_ != -1) {
    multiplexer.cleanup_cgi(stdin_fd_);
    stdin_fd_ = -1;
  }
  if (stdout_fd_ != -1) {
    multiplexer.cleanup_cgi(stdout_fd_);
    stdout_fd_ = -1;
  }
}

void CgiSession::reset() {
  state_ = DONE;
  pid_ = -1;
  stdin_fd_ = -1;
  stdout_fd_ = -1;
  conn_policy_ = CP_KEEP_ALIVE;
  body_buf_.clear();
  write_offset_ = 0;
  is_header_sent_ = false;
  cgi_last_activity_ = time(NULL);
}

void CgiSession::update_cgi_activity() { cgi_last_activity_ = time(NULL); }

CgiSession &CgiSession::operator=(const CgiSession &other) {
  (void)other;
  return *this;
}

// TODO: CONTENT_LENGTH, CONTENT_TYPE が存在しないときの扱い
