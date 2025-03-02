#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

#include "HttpRequest.hpp"

class Server {
public:
  Server(const std::map<std::string, std::vector<std::string> > &config);
  ~Server();

  void createSockets();
  void handleHttp(int clientFd, const char *buffer, int nbytes);

private:
  std::map<std::string, std::vector<std::string> > config;
  std::vector<int> listenPorts_;
  std::string public_root;
  std::string error_404;
  HttpRequest httpRequest;

  int createListenSocket(int port);
  void listenSocket(int sockfd, std::string portStr);

  Server();
  Server(const Server &src);
  Server &operator=(const Server &src);
};
