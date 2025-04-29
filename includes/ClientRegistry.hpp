#pragma once

#include <map>

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

private:
  std::map<int, Client *> clients;

  typedef std::map<int, Client *>::iterator ClientIt;
  typedef std::map<int, Client *>::const_iterator ConstClientIt;

  ClientRegistry(const ClientRegistry &other);
  ClientRegistry &operator=(const ClientRegistry &other);
};
