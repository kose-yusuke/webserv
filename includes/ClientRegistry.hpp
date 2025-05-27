#pragma once

#include <cstddef>
#include <map>
#include <vector>

class Client;
class CgiSession;

class ClientRegistry {
public:
  ClientRegistry();
  ~ClientRegistry();

  void add(int fd, Client *client);
  void remove(int fd);
  Client *get(int fd) const;
  bool has(int fd) const;
  size_t size() const;

  void add_cgi(int fd, CgiSession *session);
  void remove_cgi(int fd);
  CgiSession *get_cgi(int fd) const;
  bool has_cgi(int fd) const;

  std::vector<int> mark_timed_out_clients();
  std::vector<int> detect_unresponsive_clients() const;

private:
  std::map<int, Client *> clients_;
  std::map<int, CgiSession *> cgis_;

  typedef std::map<int, Client *>::iterator ClientIt;
  typedef std::map<int, Client *>::const_iterator ConstClientIt;
  typedef std::map<int, CgiSession *>::iterator CgiIt;
  typedef std::map<int, CgiSession *>::const_iterator ConstCgiIt;

  ClientRegistry(const ClientRegistry &other);
  ClientRegistry &operator=(const ClientRegistry &other);
};
