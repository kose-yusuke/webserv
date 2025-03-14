#include "Server.hpp"
#include "Multiplexer.hpp"
#include "ConfigParse.hpp"
#include <cstring>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>

Server::Server(const std::map<std::string, std::vector<std::string> > &config,
               const std::map<std::string, std::map<std::string, std::vector<std::string> > >& locations)
    : config(config), location_configs(locations), httpRequest(config, locations) {
    // listen
    std::map<std::string, std::vector<std::string> >::const_iterator listen_it =
        config.find("listen");
    if (listen_it == config.end() || listen_it->second.empty())
      print_error_message("Missing required key: listen");

    for (size_t i = 0; i < listen_it->second.size(); i++) {
      int port;
      std::stringstream ss(listen_it->second[i]);
      if (!(ss >> port))
        print_error_message("Invalid port number: " +
                                 listen_it->second[i]);
      listenPorts_.push_back(port);
    }

    // root
    std::map<std::string, std::vector<std::string> >::const_iterator root_it =
        config.find("root");
    if (root_it == config.end() || root_it->second.empty())
      print_error_message("Missing required key: root");
    // この辺実はいらなそう
    public_root = root_it->second[0];

    std::map<std::string, std::vector<std::string> >::const_iterator error_it =
        config.find("error_page 404");
    error_404 = (error_it != config.end() && !error_it->second.empty())
                    ? error_it->second[0]
                    : "404.html";
}

Server::~Server() {}

void Server::createSockets() {
  std::vector<int> tempFds;
  try {
    for (size_t i = 0; i < listenPorts_.size(); i++) {
      int serverFd = createListenSocket(listenPorts_[i]);
      tempFds.push_back(serverFd);
      Multiplexer::addServerFd(serverFd, this);
    }
    std::cout << "Socket created and options set successfully\n";
  } catch (...) {
    for (size_t i = 0; i < tempFds.size(); i++) {
      close(tempFds[i]);
    }
    throw;
  }
}

int Server::createListenSocket(int port) {
  int sockfd = -1, status, opt = 1;
  struct addrinfo hints, *ai, *p;
  std::stringstream ss;
  ss << port;
  std::string port_str = ss.str();

  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6 両方対応
  hints.ai_socktype = SOCK_STREAM; // stream sockets
  hints.ai_flags = AI_PASSIVE;     // serverなので fill in my IP as host

  if ((status = getaddrinfo(NULL, port_str.c_str(), &hints, &ai)) != 0) {
    throw std::runtime_error("getaddrinfo error for port " + port_str + ": " +
                             std::string(gai_strerror(status)));
  }
  // 複数の結果がgetaddrinfoに入っている。順に試す
  for (p = ai; p; p = p->ai_next) {
    sockfd = -1;
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
      close(sockfd);
      continue;
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      continue;
    }
    // socket(), setsockopt(), bind() 全部成功したらloopを抜ける
    std::cout << "Socket bound to port " << port << "\n";
    break;
  }
  freeaddrinfo(ai);
  if (!p) {
    throw std::runtime_error("Faild to prepare socket for port: " + port_str);
  }
  listenSocket(sockfd, port_str);
  return sockfd;
}

void Server::listenSocket(int sockfd, std::string portStr) {
  if (listen(sockfd, SOMAXCONN) < 0) {
    close(sockfd);
    throw std::runtime_error("Failed to listen on port: " + portStr);
  }
  std::cout << "Server is listening on port " << portStr << " (fd: " << sockfd
            << ")\n";
}

Server::Server() {}

Server::Server(const Server &src) { (void)src; }

Server &Server::operator=(const Server &src) {
  (void)src;
  return *this;
}

void Server::handleHttp(int clientFd, const char *buffer, int nbytes) {
    httpRequest.handleHttpRequest(clientFd, buffer, nbytes);
}

// Server::Server(const Server &src)
//     : config(src.config), listen_ports(src.listen_ports),
//     public_root(src.public_root),
//       error_404(src.error_404), server_fds()
// {
//     try {
//         create_sockets();  // コピー時に新しくソケットを作る
//     } catch (const std::exception& e) {
//         throw std::runtime_error("Error copying server: " +
//         std::string(e.what()));
//     }
// }

// Server& Server::operator=(const Server &src)
// {
//     if (this != &src)
//     {
//         config = src.config;
//         listen_ports = src.listen_ports;
//         public_root = src.public_root;
//         error_404 = src.error_404;

//         for (size_t i = 0; i < server_fds.size(); i++) {
//             close(server_fds[i]);
//         }
//         server_fds.clear();

//         try {
//             create_sockets();
//         } catch (const std::exception& e) {
//             throw std::runtime_error("Error copying server: " +
//             std::string(e.what()));
//         }
//     }
//     return *this;
// }
