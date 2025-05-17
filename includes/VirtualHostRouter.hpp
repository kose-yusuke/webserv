#pragma once

#include <string>
#include <vector>
#include "ConfigParse.hpp"

class Server;

class VirtualHostRouter {
public:
  VirtualHostRouter();
  ~VirtualHostRouter();

  void add(Server *s);
  Server *route_by_host(const std::string &host) const;

private:
  std::vector<Server *> servers_;
  Parse parser_;

  VirtualHostRouter(const VirtualHostRouter &other);
  VirtualHostRouter &operator=(const VirtualHostRouter &other);
};
