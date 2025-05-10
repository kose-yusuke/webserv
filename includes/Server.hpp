#pragma once

#include "types.hpp"
#include <map>
#include <string>
#include <vector>

class Server {
public:
  Server(const ConfigMap &config, const LocationMap &locations);
  Server(const Server &src);
  ~Server();

  const ConfigMap &get_config() const;
  const LocationMap &get_locations() const;

  bool matches_host(const std::string &host) const;
  bool is_default_server() const;

private:
  ConfigMap server_config;
  LocationMap location_configs;

  std::string public_root;
  std::string error_404;
  bool _is_default;

  Server();
  Server &operator=(const Server &src);
};
