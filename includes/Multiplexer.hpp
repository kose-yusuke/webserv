#pragma once

#include <map>
#include <vector>

class Server;

/**
 * Server の I/O 多重化を管理する基底クラス
 */
class Multiplexer {
public:
  static void run();

  static void addServerFd(int serverFd, Server *server);
  static void closeAllFds();

protected:
  static std::map<int, Server *> serverFdMap_;     // map: serverFd->Server*
  static std::map<int, Server *> clientServerMap_; // map: clientFd->Server*

  static void removeServerFd(int serverFd);
  static bool isInServerFdMap(int serverFd);
  static Server *getServerFromServerFdMap(int serverFd);

  static void addClientFd(int clientFd, Server *server);
  static void removeClientFd(int clientFd);
  static bool isInClientServerMap(int clientFd);
  static Server *getServerFromClientServerMap(int clientFd);

  Multiplexer();
  Multiplexer(const Multiplexer &other);
  ~Multiplexer();

private:
  Multiplexer &operator=(const Multiplexer &other);
};
