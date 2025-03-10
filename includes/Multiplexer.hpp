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
  static void run();

  static void add_server_fd(int fd, Server *server);
  static void close_all_fds();

protected:
  static std::map<int, Server *> server_map; // map: serverfd -> Server*
  static std::map<int, Client *> client_map; // map: clientfd -> Client*

  static void remove_server_fd(int fd);
  static bool is_in_server_map(int fd);
  static Server *get_server_from_map(int fd);

  static void add_client_fd(int fd, Client *client);
  static void remove_client_fd(int fd);
  static bool is_in_client_map(int fd);
  static Client *get_client_from_map(int fd);

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  ~Multiplexer();

private:
  Multiplexer &operator=(const Multiplexer &other);
};
