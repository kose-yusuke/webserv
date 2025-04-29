#pragma once

#include <cstddef>
#include <map>
#include <vector>

class Server;
class Client;
class ServerRegistry;
class ClientRegistry;

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

  void set_server_registry(ServerRegistry *registry);
  void set_client_registry(ClientRegistry *registry);

protected:
  // Singleton pattern
  static Multiplexer *instance;

  // I/O多重化処理の管理
  void process_event(int fd, bool readable, bool writable);

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  virtual ~Multiplexer();

private:
  // Registry
  ServerRegistry *server_registry;
  ClientRegistry *client_registry;

  // I/O多重化処理の補助関数
  void accept_client(int server_fd);
  void read_from_client(int client_fd);
  void write_to_client(int client_fd);
  void shutdown_write(int client_fd);
  void remove_client(int client_fd);

  // 代入禁止
  Multiplexer &operator=(const Multiplexer &other);
};

/*
try {
    add_to_client_map(client_fd, new Client(client_fd, server_fd));
  } catch (const std::exception &e) {
    logfd(LOG_ERROR, e.what(), client_fd);
    close(client_fd);
    return;
  }
  Multiplexer::get_instance().monitor_read(client_fd);
  // monitor_read(client_fd);
  logfd(LOG_DEBUG, "New connection on client fd: ", client_fd);

void ConnectionManager::remove_connection(Client *client, int client_fd) {
  LOG_DEBUG_FUNC_FD(client_fd);
  unmonitor(client_fd);
  remove_from_client_map(client_fd); // close, delete もこの中
}

*/
