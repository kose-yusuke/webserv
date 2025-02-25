#include "PollMultiplexer.hpp"
#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

void PollMultiplexer::run() {
  std::cout << "PollMultiplexer::run() called\n";
  std::vector<struct pollfd> pfds;

  pfds.reserve(serverFdMap_.size());
  addAllServerFdsToPfds(pfds);
  if (pfds.empty()) {
    throw std::runtime_error("pollfd empty");
  }
  while (true) {
    int pollCount = poll(&pfds[0], pfds.size(), 0); // non-blocking: timeout=0
    if (pollCount == -1) {
      throw std::runtime_error("poll failed");
    }
    for (size_t i = 0; i < pfds.size(); ++i) {
      if (pfds[i].revents & (POLLIN | POLLHUP)) {
        if (isInServerFdMap(pfds[i].fd)) {
          acceptClient(pfds, pfds[i].fd);
        } else {
          handleClient(pfds, pfds[i].fd);
        }
      }
    }
  }
}

void PollMultiplexer::addPfd(std::vector<struct pollfd> &pfds, int fd) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfds.push_back(pfd);
}

void PollMultiplexer::removePfd(std::vector<struct pollfd> &pfds, int fd) {
  for (std::vector<struct pollfd>::iterator it = pfds.begin(); it != pfds.end();
       ++it) {
    if (it->fd == fd) {
      pfds.erase(it);
      return;
    }
  }
}

bool PollMultiplexer::isInPfds(const std::vector<struct pollfd> &pfds, int fd) {
  for (size_t i = 0; i < pfds.size(); ++i) {
    if (pfds[i].fd == fd) {
      return true;
    }
  }
  return false;
}

void PollMultiplexer::addAllServerFdsToPfds(std::vector<struct pollfd> &pfds) {
  for (std::map<int, Server *>::iterator it = serverFdMap_.begin();
       it != serverFdMap_.end(); ++it) {
    addPfd(pfds, it->first);
  }
}

void PollMultiplexer::acceptClient(std::vector<struct pollfd> &pfds,
                                   int serverFd) {
  struct sockaddr_storage clientAddr;
  socklen_t addrlen = sizeof(clientAddr);

  int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &addrlen);
  if (clientFd == -1) {
    std::cerr << "Failed to accept incoming call\n";
    return;
  }
  std::cout << "New connection on client fd: " << clientFd << "\n";
  try {
    addClientFd(clientFd, getServerFromServerFdMap(serverFd));
    addPfd(pfds, clientFd);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    close(clientFd);
  }
}

void PollMultiplexer::handleClient(std::vector<struct pollfd> &pfds,
                                   int clientFd) {
  char buffer[1024] = {0};

  int nbytes = recv(clientFd, buffer, sizeof(buffer), 0);
  if (nbytes <= 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      return; // TODO: 確認
    }
    if (nbytes == 0) {
      std::cout << "handleClient: socket hung up: " << clientFd << "\n";
    } else {
      std::cerr << "handleClient: recv failed: " << clientFd << "\n";
    }
    close(clientFd);
    removePfd(pfds, clientFd);
    removeClientFd(clientFd);
    return;
  }
  try {
    Server *server = getServerFromClientServerMap(clientFd);
    server->handleHttpRequest(clientFd, buffer, nbytes);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    close(clientFd);
  }
}

PollMultiplexer::PollMultiplexer() {}

PollMultiplexer::PollMultiplexer(const PollMultiplexer &other)
    : Multiplexer(other) {
  (void)other;
}

PollMultiplexer::~PollMultiplexer() {}

PollMultiplexer &PollMultiplexer::operator=(const PollMultiplexer &other) {
  (void)other;
  return *this;
}
