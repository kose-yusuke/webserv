
#include "Client.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"
#include <sstream>
#include <stdexcept>

// TODO: timeout処理が必要 あとで

Client::Client(int clientfd, int serverfd)
    : fd(clientfd), server_fd(serverfd), response(),
      request(serverfd, response), parser(request), client_state(CLIENT_ALIVE),
      response_sent(0) {}

Client::~Client() {}

IOStatus Client::on_read() {
  LOG_DEBUG_FUNC();
  const int buf_size = 1024;
  char buffer[buf_size];
  ssize_t bytes_read = recv(fd, buffer, sizeof(buffer), 0);
  if (bytes_read == -1 || bytes_read == 0) {
    return IO_SHOULD_CLOSE;
  }
  parser.append_data(buffer, bytes_read);

  if (client_state == CLIENT_HALF_CLOSED) {
    return on_half_close();
  }
  return on_parse();
}

IOStatus Client::on_write() {
  LOG_DEBUG_FUNC();
  if (client_state == CLIENT_HALF_CLOSED || !has_response()) {
    return IO_WRITE_COMPLETE;
  }
  if (!current_entry) {
    current_entry = response.get_next_response();
    response_sent = 0;
  }

  ssize_t bytes_sent = send(fd, current_entry->buffer.c_str() + response_sent,
                            current_entry->buffer.size() - response_sent, 0);
  if (bytes_sent == -1 || bytes_sent == 0) {
    return IO_SHOULD_CLOSE;
  }

  response_sent += bytes_sent;
  if (response_sent < current_entry->buffer.size()) {
    return IO_CONTINUE; // 今回のsendで、response全体を送信できなかった
  }

  ConnectionPolicy conn_policy = current_entry->conn; // response送信完了
  response.pop_response();
  current_entry = 0;

  if (conn_policy == CP_WILL_CLOSE) {
    return IO_SHOULD_CLOSE;
  } else if (conn_policy == CP_MUST_CLOSE) {
    client_state = CLIENT_HALF_CLOSED;
    return IO_SHOULD_SHUTDOWN;
  }
  return has_response() ? IO_CONTINUE : IO_WRITE_COMPLETE;
}

IOStatus Client::on_half_close() {
  LOG_DEBUG_FUNC();
  while (parser.parse()) {
    if (request.get_connection_policy() == CP_WILL_CLOSE) {
      log(LOG_DEBUG, "Graceful shutdown confirmed from client.");
      return IO_SHOULD_CLOSE;
    }
    parser.clear();
  }
  return IO_CONTINUE;
}

IOStatus Client::on_parse() {
  LOG_DEBUG_FUNC();
  while (parser.parse()) {
    // trueの場合、requestの解析が完了しレスポンスを生成できる状態
    // request内でresponseクラスにresponseをpushしている
    request.handle_http_request();
    parser.clear(); // 状態をclearして, 再びheaderのparseを待機
  }
  return has_response() ? IO_READY_TO_WRITE : IO_CONTINUE;
}

bool Client::has_response() const {
  // 送信中の entry または これから送信を開始できる is_ready な entry がある
  return ((current_entry && current_entry->is_ready) ||
          response.has_next_response());
}

Client &Client::operator=(const Client &other) {
  (void)other;
  return *this;
}
