#pragma once

#include <cstddef>
#include <map>
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
  size_t size() const;

  std::vector<int> mark_timed_out_clients();
  std::vector<int> detect_unresponsive_clients() const;

private:
  std::map<int, Client *> clients_;

  typedef std::map<int, Client *>::iterator ClientIt;
  typedef std::map<int, Client *>::const_iterator ConstClientIt;

  ClientRegistry(const ClientRegistry &other);
  ClientRegistry &operator=(const ClientRegistry &other);
};
