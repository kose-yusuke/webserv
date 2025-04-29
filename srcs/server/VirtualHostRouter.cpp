#include "VirtualHostRouter.hpp"
#include "Server.hpp"
#include <cstddef>

VirtualHostRouter::VirtualHostRouter() : servers() {}

VirtualHostRouter::~VirtualHostRouter() {
  for (size_t i = 0; i < servers.size(); ++i) {
    delete servers[i];
  }
}

void VirtualHostRouter::add(Server *s) { servers.push_back(s); }

Server *VirtualHostRouter::route_by_host(const std::string &host) const {
  size_t colon_pos = host.find(':');
  std::string host_name;
  if (colon_pos != std::string::npos) {
    host_name = host.substr(0, colon_pos);
  } else {
    host_name = host;
  }
  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i]->matches_host(host_name)) {
      return servers[i];
    }
  }
  return servers.empty() ? NULL : servers[0];
}

VirtualHostRouter &
VirtualHostRouter::operator=(const VirtualHostRouter &other) {
  (void)other;
  return *this;
}
