#pragma once

#include "types.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

// #include "HttpRequest.hpp"

class Server {
public:
  Server(const ConfigMap &config, const LocationMap &locations);
  ~Server();

  void createSockets();

  const ConfigMap &get_config() const;
  const LocationMap &get_locations() const;

private:
  ConfigMap server_config;
  LocationMap location_configs;

  std::vector<int> listenPorts_;
  std::string public_root;
  std::string error_404;
  // HttpRequestはServerではなくClientに持たせたいためコメントアウト
  // HttpRequest httpRequest;

  int createListenSocket(int port);
  void listenSocket(int sockfd, std::string portStr);

  Server();
  Server(const Server &src);
  Server &operator=(const Server &src);
};
