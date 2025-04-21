#pragma once

#include <cstddef>
#include <map>
#include <vector>

class Server;
class Client;

/**
 * Server の I/O 多重化を管理する基底クラス
 */
class Multiplexer {
public:
  // Singleton pattern
  static Multiplexer &get_instance();
  static void delete_instance();

  virtual void run() = 0;
  void add_to_server_map(int fd, Server *server);
  Server *get_server_from_map(int fd) const;

protected:
  // Singleton pattern
  static Multiplexer *instance;

  // server_mapの管理
  void remove_from_server_map(int fd);
  bool is_in_server_map(int fd) const;
  size_t get_num_servers() const;

  // client_mapの管理
  void add_to_client_map(int fd, Client *client);
  void remove_from_client_map(int fd);
  bool is_in_client_map(int fd) const;
  Client *get_client_from_map(int fd) const;
  size_t get_num_clients() const;

  // I/O多重化処理の管理
  void initialize_fds();
  void process_event(int fd, bool readable, bool writable);

  // 監視fd管理用の純粋仮想関数
  virtual void monitor_read(int fd) = 0;
  virtual void monitor_write(int fd) = 0;
  virtual void unmonitor_write(int fd) = 0;
  virtual void unmonitor(int fd) = 0;

  // 解放処理（fd close & clientのみインスタンス削除）
  void free_all_fds();

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  virtual ~Multiplexer();

private:
  // mapのiterator型定義
  typedef std::map<int, Server *>::iterator ServerIt;
  typedef std::map<int, Client *>::iterator ClientIt;
  typedef std::map<int, Server *>::const_iterator ConstServerIt;
  typedef std::map<int, Client *>::const_iterator ConstClientIt;

  // client, server の fd管理
  std::map<int, Server *> server_map; // serverfd -> Server*
  std::map<int, Client *> client_map; // clientfd -> Client*

  // I/O多重化処理の補助関数
  void accept_client(int server_fd);
  void read_from_client(int client_fd);
  void write_to_client(int client_fd);
  void shutdown_write(int client_fd);
  void remove_client(int client_fd);

  // 代入禁止
  Multiplexer &operator=(const Multiplexer &other);
};
