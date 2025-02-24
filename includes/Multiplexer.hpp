#pragma once

#include <map>
#include <vector>

class Server;

class Multiplexer {
public:
  static void run(); // OSごとに合わせた Multiplexer の呼び出しを担当

protected:
  static std::map<int, Server *> serverFdMap_;     // serverFd->Server設定
  static std::map<int, Server *> clientServerMap_; // clientFd->Server設定

  // serverFdMap_
  static void addServerFd(int serverFd, Server *server);
  static void removeServerFd(int serverFd);
  static bool isInServerFdMap(int serverFd);
  static Server *getServerFromServerFdMap(int serverFd);

  // clientServerMap_
  static void addClientFd(int clientFd, Server *server);
  static void removeClientFd(int clientFd);
  static bool isInClientServerMap(int clientFd);
  static Server *getServerFromClientServerMap(int clientFd);

  Multiplexer();
  virtual ~Multiplexer();

private:
  Multiplexer(const Multiplexer &other);
  Multiplexer &operator=(const Multiplexer &other);
};
