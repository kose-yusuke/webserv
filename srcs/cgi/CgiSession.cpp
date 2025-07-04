#include "CgiSession.hpp"
#include "Logger.hpp"
#include "Multiplexer.hpp"
#include <iostream>
#include <signal.h>

static const int k_timeout_sec = 10;

CgiSession::CgiSession(int client_fd)
    : parser_(), builder_(), client_fd_(client_fd), state_(CGI_IDLE), pid_(-1),
      stdin_fd_(-1), stdout_fd_(-1), in_buf_(), in_off_(0),
      cgi_last_activity_(time(NULL)), client_alive_(true) {
  log(LOG_DEBUG, "CGI constructor called");
}

// forkしたプロセスとCGI fdのclean up責務はCgiSession
CgiSession::~CgiSession() {
  if (pid_ != -1 && waitpid(pid_, NULL, WNOHANG) == 0) {
    kill(pid_, SIGKILL);
    // 回収はsignal(SIGCHLD, handle_sigchld);が動作する
  }
  terminate_cgi_fds();
}

int CgiSession::get_stdin_fd() const { return stdin_fd_; }

int CgiSession::get_stdout_fd() const { return stdout_fd_; }

void CgiSession::mark_client_dead() { client_alive_ = false; }

bool CgiSession::is_client_alive() const { return client_alive_; }

// CGIのIOが進行中か（読み書き途中）
bool CgiSession::is_processing() const {
  return (state_ == CGI_WRITING || state_ == CGI_READING);
}

// 正常に読み取り完了したか
bool CgiSession::is_eof() const { return (state_ == CGI_EOF); }

// 異常終了したか（エラー or タイムアウト）
bool CgiSession::is_failed() const {
  return (state_ == CGI_ERROR || state_ == CGI_TIMED_OUT);
}

bool CgiSession::is_session_completed() const {
  return (is_eof() && parser_.is_done());
}

// Time-outしたか
bool CgiSession::is_cgi_timeout(time_t now) const {
  if (state_ == CGI_IDLE || state_ == CGI_ERROR) {
    return false;
  }
  return (state_ == CGI_TIMED_OUT || now - cgi_last_activity_ > k_timeout_sec);
}

void CgiSession::handle_cgi_request(HttpRequest &request,
                                    const std::string &cgi_path) {
  LOG_DEBUG_FUNC();
  int input_pipe[2];
  int output_pipe[2];
  const std::string &method = request.get_method();
  const std::string &target = request.get_path();

  builder_.set_connection_policy(request.get_connection_policy());
  in_buf_ = request.get_body();
  in_off_ = 0;

  if (pipe(input_pipe) == -1) {
    throw std::runtime_error("pipe failed: " + std::string(strerror(errno)));
  }
  if (pipe(output_pipe) == -1) {
    close(input_pipe[0]);
    close(input_pipe[1]);
    throw std::runtime_error("pipe failed: " + std::string(strerror(errno)));
  }

  pid_ = fork();
  if (pid_ < 0) {
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    throw std::runtime_error("fork failed: " + std::string(strerror(errno)));
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
    std::string contentLength = request.get_header_value("Content-Length");
    std::string contentLengthStr = "CONTENT_LENGTH=" + contentLength;
    std::string requestMethodStr = "REQUEST_METHOD=" + method;
    std::string contentTypeStr =
        "CONTENT_TYPE=" + request.get_header_value("Content-Type");
    std::string queryString = "QUERY_STRING=";

    if (method == "GET") {
      size_t pos = target.find('?');
      if (pos != std::string::npos) {
        queryString += target.substr(pos + 1);
      }
    }

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
      close(stdin_fd_);
      close(stdout_fd_);
      throw std::runtime_error("fcntl failed: " + std::string(strerror(errno)));
    }

    // ClientRegistryに登録し、stdin_fd_の監視を開始する
    Multiplexer &multiplexer = Multiplexer::get_instance();
    multiplexer.register_cgi_fd(stdin_fd_, this);
    multiplexer.monitor_pipe_write(stdin_fd_);
    state_ = CGI_WRITING;
  }
}

// Client::on_write() -> HttpTransaction -> で呼ばれる関数
void CgiSession::build_response(HttpResponse &response) {
  LOG_DEBUG_FUNC();
  if (is_failed()) {
    builder_.build_error_response(response, 500);
    return;
  }

  bool eof = is_eof();

  if (parser_.parse(eof) && !builder_.is_sent()) {
    // なにか送るものがある
    builder_.apply(parser_);
    builder_.build_response(response, eof);
  }
}

void CgiSession::build_error_response(HttpResponse &response, int status_code,
                                      ConnectionPolicy policy) {
  LOG_DEBUG_FUNC();
  builder_.build_error_response(response, status_code, policy);
}

void CgiSession::close_fd(int fd) {
  if (fd == -1 || (fd != stdin_fd_ && fd != stdout_fd_)) {
    logfd(LOG_WARNING, "Invalid CGI fd to close: ", fd);
    return;
  }
  if (close(fd) == -1) {
    logfd(LOG_WARNING, "Failed to close cgi fd: ", fd);
  }
  if (fd == stdin_fd_) {
    stdin_fd_ = -1;
  }
  if (fd == stdout_fd_) {
    stdout_fd_ = -1;
  }
}

CgiIOStatus CgiSession::on_cgi_write() {
  LOG_DEBUG_FUNC();
  if (!is_processing()) {
    return CGI_IO_ERROR;
  }
  if (in_buf_.empty()) {
    state_ = CGI_READING;
    return CGI_IO_WRITE_COMPLETE;
  }
  ssize_t bytes_write =
      write(stdin_fd_, &in_buf_[0] + in_off_, in_buf_.size() - in_off_);
  if (bytes_write == -1) {
    state_ = CGI_ERROR;
    terminate_pid();
    return CGI_IO_ERROR;
  }
  update_cgi_activity();
  in_off_ += bytes_write;
  if (in_off_ < in_buf_.size()) {
    return CGI_IO_CONTINUE;
  }
  in_buf_.clear();
  in_off_ = 0;
  state_ = CGI_READING;
  return CGI_IO_WRITE_COMPLETE;
}

CgiIOStatus CgiSession::on_cgi_read() {
  LOG_DEBUG_FUNC();
  if (!is_processing()) {
    return CGI_IO_ERROR;
  }
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = read(stdout_fd_, buffer, buf_size);
  if (bytes_read == -1) {
    state_ = CGI_ERROR;
    terminate_pid();
    return CGI_IO_ERROR;
  }
  update_cgi_activity();
  if (bytes_read == 0) {
    state_ = CGI_EOF;
    return CGI_IO_READ_COMPLETE;
  }
  parser_.append(buffer, bytes_read);
  return CGI_IO_CONTINUE;
}

void CgiSession::on_cgi_timeout() {
  if (is_terminal_state()) {
    return;
  }
  LOG_DEBUG_FUNC();
  state_ = CGI_TIMED_OUT;
  terminate_pid();
  terminate_cgi_fds();
}

void CgiSession::terminate_pid() {
  if (pid_ == -1) {
    return;
  }
  LOG_DEBUG_FUNC();
  kill(pid_, SIGTERM);
}

void CgiSession::terminate_cgi_fds() {
  LOG_DEBUG_FUNC();

  Multiplexer &multiplexer = Multiplexer::get_instance();

  if (stdin_fd_ != -1) {
    multiplexer.cleanup_cgi(stdin_fd_);
  }
  if (stdout_fd_ != -1) {
    multiplexer.cleanup_cgi(stdout_fd_);
  }
}

bool CgiSession::is_terminal_state() const {
  return (state_ == CGI_ERROR || state_ == CGI_TIMED_OUT);
}

void CgiSession::update_cgi_activity() { cgi_last_activity_ = time(NULL); }

CgiSession &CgiSession::operator=(const CgiSession &other) {
  (void)other;
  return *this;
}
