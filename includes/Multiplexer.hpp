#pragma once

#include <cstddef>
#include <map>
#include <sys/types.h>
#include <vector>

class Server;
class Client;
class CgiSession;
class ServerRegistry;
class ClientRegistry;
class CgiRegistry;

/**
 * Server の I/O 多重化を管理する基底クラス
 */
class Multiplexer {
public:
  // Singleton pattern
  static Multiplexer &get_instance();
  static void delete_instance();

  virtual void run() = 0;

  // 監視fd管理用の純粋仮想関数
  virtual void monitor_read(int fd) = 0;
  virtual void monitor_write(int fd) = 0;
  virtual void unmonitor_write(int fd) = 0;
  virtual void unmonitor(int fd) = 0;

  virtual void monitor_pipe_read(int fd) = 0;
  virtual void monitor_pipe_write(int fd) = 0;

  void set_server_registry(ServerRegistry *registry);
  void set_client_registry(ClientRegistry *registry);
  void set_cgi_registry(CgiRegistry *registry);

  void register_cgi_fd(int fd, CgiSession *session);
  void cleanup_cgi(int cgi_fd);

protected:
  // Singleton pattern
  static Multiplexer *instance_;

  static const int k_timeout_ms_;

  // I/O多重化処理の管理
  void process_event(int fd, bool readable, bool writable);
  void handle_timeouts();

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  virtual ~Multiplexer();

private:
  // Registry
  ServerRegistry *server_registry_;
  ClientRegistry *client_registry_;
  CgiRegistry *cgi_registry_;

  // I/O多重化処理の補助関数
  void accept_client(int server_fd);
  void read_from_client(int client_fd);
  void write_to_client(int client_fd);
  void shutdown_write(int client_fd);
  void cleanup_client(int client_fd);

  // CGIのfdを扱う関数
  void read_from_cgi(int cgi_stdout);
  void write_to_cgi(int cgi_stdin);

  // 代入禁止
  Multiplexer &operator=(const Multiplexer &other);
};
