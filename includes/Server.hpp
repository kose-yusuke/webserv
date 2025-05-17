#pragma once

#include "types.hpp"
#include <cstddef>
#include <iostream>
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
  const std::vector<std::string> &get_server_names() const;

  bool is_default_server() const;

private:
  ConfigMap server_config_;
  LocationMap location_configs_;

  std::string public_root_;
  std::string error_404_;
  bool _is_default;

  Server();
  Server &operator=(const Server &src);
};
