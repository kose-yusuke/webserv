#include "Server.hpp"
#include "Multiplexer.hpp"
#include "config_parse.hpp"
#include <cstring>
#include <netdb.h>
#include <sstream>
#include <sys/socket.h>
#include <sys/types.h>

Server::Server(const std::map<std::string, std::vector<std::string> > &config)
    : config(config) {
  try {
    // listen
    std::map<std::string, std::vector<std::string> >::const_iterator listen_it =
        config.find("listen");
    if (listen_it == config.end() || listen_it->second.empty())
      throw std::runtime_error("Missing required key: listen");

    for (size_t i = 0; i < listen_it->second.size(); i++) {
      int port;
      std::stringstream ss(listen_it->second[i]);
      if (!(ss >> port))
        throw std::runtime_error("Invalid port number: " +
                                 listen_it->second[i]);
      listenPorts_.push_back(port);
    }

    // root
    std::map<std::string, std::vector<std::string> >::const_iterator root_it =
        config.find("root");
    if (root_it == config.end() || root_it->second.empty()) {
      throw std::runtime_error("Missing required key: root");
    }
    // この辺実はいらなそう
    public_root = root_it->second[0];

    std::map<std::string, std::vector<std::string> >::const_iterator error_it =
        config.find("error_page 404");
    error_404 = (error_it != config.end() && !error_it->second.empty())
                    ? error_it->second[0]
                    : "404.html";

  } catch (const std::exception &e) {
    throw std::runtime_error(std::string("Error initializing server: ") +
                             e.what());
  }
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

void Server::handleHttpRequest(int clientFd, const char *buffer, int nbytes) {

  (void)nbytes;
  std::string method, path, version;
  if (!parse_http_request(buffer, method, path, version)) {
    send_error_response(clientFd, 400, "Bad Request");
    return;
  }

  std::cout << "HTTP Method: " << method << ", Path: " << path << "\n";

  if (method == "GET") {
    handle_get_request(clientFd, path);
  } else if (method == "POST") {
    handle_post_request(clientFd, buffer);
  } else if (method == "DELETE") {
    handle_delete_request();
  } else {
    send_error_response(clientFd, 405, "Method Not Allowed");
  }
}

// error page用のclassとファイル
void Server::send_custom_error_page(int client_socket, int status_code,
                                    const std::string &error_page) {
  try {
    std::string file_content = read_file("./public/" + error_page);

    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " ";
    if (status_code == 404) {
      response << "Not Found";
    } else if (status_code == 405) {
      response << "Method Not Allowed";
    }
    response << "\r\n";
    response << "Content-Length: " << file_content.size() << "\r\n";
    response << "Content-Type: text/html\r\n\r\n";
    response << file_content;

    send(client_socket, response.str().c_str(), response.str().size(), 0);
  } catch (const std::exception &e) {
    std::ostringstream fallback;
    fallback << "HTTP/1.1 " << status_code << " ";
    if (status_code == 404) {
      fallback << "Not Found";
    } else if (status_code == 405) {
      fallback << "Method Not Allowed";
    }
    fallback << "\r\nContent-Length: 9\r\n\r\nNot Found";
    send(client_socket, fallback.str().c_str(), fallback.str().size(), 0);
  }
}

// request handler class
bool Server::parse_http_request(const std::string &request, std::string &method,
                                std::string &path, std::string &version) {
  std::istringstream request_stream(request);
  if (!(request_stream >> method >> path >> version)) {
    return false;
  }
  return true;
}

void Server::handle_get_request(int client_socket, std::string path) {
  if (path == "/") {
    path = "/index.html";
  }

  std::string file_path = public_root + path;
  try {
    std::cout << file_path << std::endl;
    std::string file_content = read_file(file_path);
    // std::string mime_type = get_mime_type(file_path);

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Length: " << file_content.size() << "\r\n";
    // response << "Content-Type: " << mime_type << "\r\n\r\n";
    response << "Content-Type: text/html\r\n\r\n";
    response << file_content;

    send(client_socket, response.str().c_str(), response.str().size(), 0);

  } catch (const std::exception &e) {
    send_custom_error_page(client_socket, 404, error_404);
  }
}

// ファイルを保存する
void Server::handle_post_request(int client_socket,
                                 const std::string &request) {
  size_t body_start = request.find("\r\n\r\n");
  if (body_start == std::string::npos) {
    send_error_response(client_socket, 400, "Bad Request");
    return;
  }

  std::string body = request.substr(body_start + 4); // ボディ部分を抽出
  std::cout << "Received POST body: " << body << "\n";

  std::ostringstream response;
  response << "HTTP/1.1 200 OK\r\n";
  response << "Content-Length: " << body.size() << "\r\n";
  response << "Content-Type: text/plain\r\n\r\n";
  response << body;

  send(client_socket, response.str().c_str(), response.str().size(), 0);
}

// ファイルを削除する
void Server::handle_delete_request() {}

void Server::send_error_response(int client_socket, int status_code,
                                 const std::string &message) {
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
  response << "Content-Length: " << message.size() << "\r\n";
  response << "Content-Type: text/plain\r\n\r\n";
  response << message;

  send(client_socket, response.str().c_str(), response.str().size(), 0);
}

Server::Server() {}

Server::Server(const Server &src) { (void)src; }

Server &Server::operator=(const Server &src) {
  (void)src;
  return *this;
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
