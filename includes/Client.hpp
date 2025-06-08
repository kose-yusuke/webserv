#pragma once

#include "HttpTransaction.hpp"
#include <ctime>
#include <string>

class VirtualHostRouter;

enum ClientState {
  CLIENT_ALIVE,         // 通常
  CLIENT_CGI_TIMED_OUT, // cgi Time-out; `time out`レスポンス未準備
  CLIENT_TIMED_OUT,     // Time-out; `time out`レスポンスの送信待機
  CLIENT_HALF_CLOSED,   // SHUT_WR 済み; `time out`レスポンス送信済

  // NOTE:
  // TIMED_OUTはレスポンスを送り次第, fd を半閉して HALF_CLOSEDになる
  // さらに EOF or `Connection: close` の受領を待って、fd を close する
  // closeされたclientはdeleteされるので、CLIENT_CLOSEDのstateは省略
};

/*
Client: ソケットの管理、TCP接続の状態管理、Time-out処理
*/
class Client {
public:
  Client(int clientfd, const VirtualHostRouter *router);
  ~Client();

  int get_fd() const;

  IOStatus on_read();
  IOStatus on_write();
  IOStatus on_timeout();

  bool is_timeout(time_t now) const;
  bool is_unresponsive(time_t now) const;
  void set_cgi_timeout_status();

private:
  int fd_;
  ClientState state_;
  time_t timeout_sec_;
  time_t last_activity_;

  HttpTransaction transaction_;

  void update_activity();

  Client(const Client &other);
  Client &operator=(const Client &other);
};
