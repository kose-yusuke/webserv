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
  virtual void run() = 0; // TODO: 確認 run時のmap.empty()チェック

  void add_server_fd(int fd, Server *server); // 新規serverを追加

protected:
  typedef std::map<int, Server *>::iterator ServerIt;
  typedef std::map<int, Client *>::iterator ClientIt;

  std::map<int, Server *> server_map; // map: serverfd -> Server*
  std::map<int, Client *> client_map; // map: clientfd -> Client*

  void remove_server_fd(int fd);
  bool is_in_server_map(int fd);
  Server *get_server_from_map(int fd);

  void add_client_fd(int fd, Client *client);
  void remove_client_fd(int fd);
  bool is_in_client_map(int fd);
  Client *get_client_from_map(int fd);

  void free_all_fds();

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  virtual ~Multiplexer();

private:
  Multiplexer &operator=(const Multiplexer &other);
};

// TODO: 派生クラスの
// Multiplexer &init_multiplexer();
