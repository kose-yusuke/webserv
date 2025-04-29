#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

class Server;
class VirtualHostRouter;

struct ListenEntry {
  int fd;
  std::string ip;
  std::string port;
  VirtualHostRouter *virtual_host_router;
};

class ServerRegistry {
public:
  ServerRegistry();
  ~ServerRegistry();

  void add(const Server &s);
  void remove(int fd);
  bool has(int fd) const;
  size_t size() const;

  void initialize();

  const VirtualHostRouter *get_router(int fd) const;

private:
  std::vector<ListenEntry> entries;

  typedef std::vector<ListenEntry>::iterator ListenEntryIt;
  typedef std::vector<ListenEntry>::const_iterator ConstListenEntryIt;

  void create_new_listen_target(const std::string &ip, const std::string &port,
                                const Server &s);

  ServerRegistry(const ServerRegistry &other);
  ServerRegistry &operator=(const ServerRegistry &other);
};
