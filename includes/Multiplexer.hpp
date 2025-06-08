#pragma once

#include <cstddef>
#include <map>
#include <vector>

class Server;
class Client;
class ServerRegistry;
class ClientRegistry;
class CgiSession;
class ZombieRegistry;

/**
 * Server の I/O 多重化を管理する基底クラス
 */
class Multiplexer {
public:
  // Singleton pattern
  static Multiplexer &get_instance();
  static void delete_instance();

  virtual void run() = 0;

  // TODO: 可視性の確認
  // 監視fd管理用の純粋仮想関数
  virtual void monitor_read(int fd) = 0;
  virtual void monitor_write(int fd) = 0;
  virtual void unmonitor_write(int fd) = 0;
  virtual void unmonitor(int fd) = 0;

  virtual void monitor_pipe_read(int fd) = 0;
  virtual void monitor_pipe_write(int fd) = 0;

  void set_server_registry(ServerRegistry *registry);
  void set_client_registry(ClientRegistry *registry);
  void set_zombie_registry(ZombieRegistry *registry);

  void register_cgi_fds(int stdin_fd, int stdout_fd, CgiSession *session);
  void cleanup_cgi(int cgi_fd);
  void track_zombie(pid_t pid);

protected:
  // Singleton pattern
  static Multiplexer *instance_;

  static const int k_timeout_ms_;

  // I/O多重化処理の管理
  void process_event(int fd, bool readable, bool writable);
  void handle_timeouts();
  void handle_zombies();

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  virtual ~Multiplexer();

private:
  // Registry
  ServerRegistry *server_registry_;
  ClientRegistry *client_registry_;
  ZombieRegistry *zombie_registry_;

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
