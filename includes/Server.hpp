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

class Server {
public:
  Server(const std::map<std::string, std::vector<std::string> > &config);
  ~Server();

  void createSockets();
  void handleHttpRequest(int clientFd, const char *buffer, int nbytes);

private:
  std::map<std::string, std::vector<std::string> > config;
  std::vector<int> listenPorts_;
  std::string public_root;
  std::string error_404;

  int createListenSocket(int port);
  void listenSocket(int sockfd, std::string portStr);

  void handle_get_request(int clientFd, std::string path);
  void handle_post_request(int clientFd, const std::string &request);
  void handle_delete_request();

  bool parse_http_request(const std::string &request, std::string &method,
                          std::string &path, std::string &version);
  void send_error_response(int clientFd, int status_code,
                           const std::string &message);
  void send_custom_error_page(int clientFd, int status_code,
                              const std::string &file_path);

  Server();
  Server(const Server &src);
  Server &operator=(const Server &src);
};
