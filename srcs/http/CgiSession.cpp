#include "CgiSession.hpp"
#include "Multiplexer.hpp"
#include <signal.h>

CgiSession::CgiSession(HttpRequest &request, HttpResponse &response, int fd)
    : request_(request), response_(response), client_fd_(fd), state_(DONE) {}

CgiSession::~CgiSession() {}

int CgiSession::get_stdin_fd() const { return stdin_fd_; }

int CgiSession::get_stdout_fd() const { return stdout_fd_; }

int CgiSession::get_client_fd() const { return client_fd_; }

CgiSession &CgiSession::operator=(const CgiSession &other) {
  (void)other;
  return *this;
}

void CgiSession::handle_cgi_request(const std::string &cgi_path,
                                    std::vector<char> body_data,
                                    std::string method, std::string path) {
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
  }
}

CgiIOStatus CgiSession::on_write() {
  if (body_buf_.empty()) {
    state_ = WAIT_READ_OUTPUT;
    return CGI_IO_WRITE_COMPLETE;
  }
  ssize_t bytes_write = write(stdin_fd_, &body_buf_[0] + write_offset_,
                              body_buf_.size() - write_offset_);
  if (bytes_write == -1) {
    state_ = ERROR;
    return CGI_IO_ERROR; // エラーかな？
  }
  update_cgi_activity(); // TODO:
  write_offset_ += bytes_write;
  if (write_offset_ < body_buf_.size()) {
    return CGI_IO_CONTINUE;
  }
  body_buf_.clear();
  write_offset_ = 0;
  state_ = WAIT_READ_OUTPUT;
  return CGI_IO_WRITE_COMPLETE;
}

CgiIOStatus CgiSession::on_read() {
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = read(stdout_fd_, buffer, buf_size);
  if (bytes_read == -1) {
    state_ = ERROR;
    return CGI_IO_ERROR;
  }
  update_cgi_activity();
  if (bytes_read == 0) {
    // closeやunregisterは呼び出し側で行う？
    state_ = WAIT_CHILD_EXIT;
    return CGI_IO_READ_COMPLETE;
  }
  output_buf_.append(buffer, bytes_read);
  return CGI_IO_CONTINUE;
}

bool CgiSession::on_done() {
  int status;
  if (waitpid(pid_, &status, WNOHANG) == 0) {
    return false;
  }
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    response_.generate_response(200, output_buf_, "text/html",
                                request_.get_connection_policy());
  } else {
    request_.set_connection_policy(CP_MUST_CLOSE);
    response_.generate_error_response(500, "CGI Execution Failed",
                                      request_.get_connection_policy());
  }
  return true;
}

// TODO: timeoutはclient側でhandling？あるいはここ？

void CgiSession::set_cgi_start_time() { cgi_start_time_ = time(NULL); }

void CgiSession::update_cgi_activity() { cgi_last_activity_ = time(NULL); }

// unregister（close含む）とunmonitorは呼び出し側で行う

// いまの設計なら「Clientがtimeout判定して、CgiSession::on_timeout()
// を呼び出す」方式が一番すっきり
