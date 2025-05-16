#include "Server.hpp"
#include "ConfigParse.hpp"
#include <algorithm>

// TODO: 確認
Server::Server(const ConfigMap &config, const LocationMap &locations)
    : server_config_(config), location_configs_(locations) {

  ConstConfigIt error_it = config.find("error_page 404");
  error_404_ = (error_it != config.end() && !error_it->second.empty())
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
}

// TODO: 確認
Server::Server(const Server &src) {
  server_config_ = src.server_config_;
  location_configs_ = src.location_configs_;
  public_root_ = src.public_root_;
  error_404_ = src.error_404_;
  _is_default = src._is_default;
}

Server::~Server() {}

const ConfigMap &Server::get_config() const { return server_config_; }

const LocationMap &Server::get_locations() const { return location_configs_; }

const std::vector<std::string> &Server::get_server_names() const {
  static const std::vector<std::string> k_empty;
  ConstConfigIt it = server_config_.find("server_name");
  return (it == server_config_.end()) ? k_empty : it->second;
}

bool Server::is_default_server() const { return _is_default; }

Server &Server::operator=(const Server &src) {
  (void)src;
  return *this;
}
