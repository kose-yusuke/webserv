#include "Server.hpp"
#include "ConfigParse.hpp"
#include <algorithm>

// TODO: 確認
Server::Server(const ConfigMap &config, const LocationMap &locations)
    : server_config(config), location_configs(locations) {

  ConstConfigIt error_it = config.find("error_page 404");
  error_404 = (error_it != config.end() && !error_it->second.empty())
                  ? error_it->second[0]
                  : "404.html";
  
  ConstConfigIt listen_it = config.find("listen");
  if (listen_it != config.end()) {
    const std::vector<std::string> &tokens = listen_it->second;
    for (size_t i = 0; i < tokens.size(); ++i) {
      if (tokens[i] == "default_server") {
        _is_default = true;
        break;
      }
    }
  }

  if (_is_default)
    std::cout << "[DEBUG] default_server is true for config with server_name: "
              << (config.find("server_name") != config.end() ? config.find("server_name")->second[0] : "(none)")
              << std::endl;
  else
    std::cout << "[DEBUG] default_server is false for config with server_name: "
              << (config.find("server_name") != config.end() ? config.find("server_name")->second[0] : "(none)")
              << std::endl;
}

// TODO: 確認
Server::Server(const Server &src) {
  server_config = src.server_config;
  location_configs = src.location_configs;
  public_root = src.public_root;
  error_404 = src.error_404;
  _is_default = src._is_default;
}

Server::~Server() {}

const ConfigMap &Server::get_config() const { return server_config; }

const LocationMap &Server::get_locations() const { return location_configs; }

// TODO: wildcard server_name への対応（ ex. *.example.com ）
bool Server::matches_host(const std::string &host_name) const {
  ConstConfigIt it = server_config.find("server_name");
  if (it == server_config.end()) {
    return false;
  }
  const std::vector<std::string> &names = it->second;
  return std::find(names.begin(), names.end(), host_name) != names.end();
}

Server &Server::operator=(const Server &src) {
  (void)src;
  return *this;
}

bool Server::is_default_server() const {
  return _is_default;
}