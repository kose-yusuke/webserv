#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <vector>

class Client;

class ClientRegistry {
public:
  ClientRegistry();
  ~ClientRegistry();

  void add(int fd, Client *client);
  void remove(int fd);
  Client *get(int fd) const;
  bool has(int fd) const;

  std::vector<int> mark_timed_out_clients();
  std::vector<int> detect_unresponsive_clients() const;

private:
  std::map<int, Client *> fd_to_clients_;

  typedef std::map<int, Client *>::iterator ClientIt;
  typedef std::map<int, Client *>::const_iterator ConstClientIt;

  ClientRegistry(const ClientRegistry &other);
  ClientRegistry &operator=(const ClientRegistry &other);
};
