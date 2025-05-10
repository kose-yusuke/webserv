#pragma once

#include <string>
#include <vector>

class Server;

class VirtualHostRouter {
public:
  VirtualHostRouter();
  ~VirtualHostRouter();

  void add(Server *s);
  Server *route_by_host(const std::string &host) const;

private:
  std::vector<Server *> servers;

  VirtualHostRouter(const VirtualHostRouter &other);
  VirtualHostRouter &operator=(const VirtualHostRouter &other);
  void move_default_to_front();
};
