#include "SocketBuilder.hpp"
#include "Logger.hpp"
#include "unistd.h"
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

int SocketBuilder::create_socket(const std::string &ip,
                                 const std::string &port) {
  int sockfd = -1, status, opt = 1;
  struct addrinfo hints, *ai, *p;

  std::string clean_port = port;
  size_t space_pos = port.find(' ');
  if (space_pos != std::string::npos) {
      clean_port = port.substr(0, space_pos);
  }

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6 両方対応
  hints.ai_socktype = SOCK_STREAM; // stream sockets
  hints.ai_flags = AI_PASSIVE;     // fill in my IP as host

  if ((status = getaddrinfo(ip.c_str(), clean_port.c_str(), &hints, &ai)) != 0) {
    throw std::runtime_error("getaddrinfo error for " + ip + ":" + clean_port +
                             ", status: " + std::string(gai_strerror(status)));
  }

  // 複数の結果がgetaddrinfoに入っている。順に試す
  for (p = ai; p; p = p->ai_next) {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      continue;
    }
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK) == -1 ||
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
        bind(sockfd, p->ai_addr, p->ai_addrlen) == -1 ||
        listen(sockfd, SOMAXCONN) == -1) {
      close(sockfd);
      continue;
    }
    break; // 成功
  }
  freeaddrinfo(ai);
  if (!p) {
    throw std::runtime_error("Failed to prepare socket for " + ip + ":" + clean_port);
  }
  logfd(LOG_INFO, "Now listening on " + ip + ":" + clean_port + " fd: ", sockfd);
  return sockfd;
}
