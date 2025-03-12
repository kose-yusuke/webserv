#pragma once

#include <map>
#include <vector>

class Server;
class Client;

/**
 * Server の I/O 多重化を管理する基底クラス
 */
class Multiplexer {
public:
  static Multiplexer &get_instance();
  static void delete_instance();

  virtual void run() = 0; // TODO: 確認 run時のmap.empty()チェック

  // TODO: serverをまとめて追加する方法について再検討
  void add_to_server_map(int fd, Server *server);

protected:
  static Multiplexer *instance;

  typedef std::map<int, Server *>::iterator ServerIt;
  typedef std::map<int, Client *>::iterator ClientIt;

  std::map<int, Server *> server_map; // serverfd -> Server*
  std::map<int, Client *> client_map; // clientfd -> Client*

  void remove_from_server_map(int fd);
  bool is_in_server_map(int fd);
  Server *get_server_from_map(int fd);

  void add_to_client_map(int fd, Client *client);
  void remove_from_client_map(int fd);
  bool is_in_client_map(int fd);
  Client *get_client_from_map(int fd);

  // void initialize_fds() = 0;

  // void add_to_read_fds(int fd) = 0;
  // void remove_from_read_fds(int fd) = 0;
  // bool is_fd_readable(int fd) = 0;

  // void add_to_write_fds(int fd) = 0;
  // void remove_from_write_fds(int fd) = 0;
  // bool is_fd_writable(int fd) = 0;

  // void accept_client(int server_fd) = 0;
  // void handle_client(int client_fd) = 0;

  void free_all_fds();

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  virtual ~Multiplexer();

private:
  Multiplexer &operator=(const Multiplexer &other);
};
