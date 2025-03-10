#include "SelectMultiplexer.hpp"
#include "Server.hpp"
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void SelectMultiplexer::run() {
  std::cout << "SelectMultiplexer::run() called\n";
  fd_set mainFds;
  int maxFd = -1;
  addAllServerFdsToFdSet(mainFds, maxFd);
  while (true) {
    fd_set readFds = mainFds;
    int activity = select(maxFd + 1, &readFds, NULL, NULL, NULL);
    if (activity < 0 && errno != EINTR) {
      std::cerr << "Error: select() failed with errno " << errno << " ("
                << strerror(errno) << ")\n";
      continue;
    }
    for (int eventFd = 0; eventFd <= maxFd; ++eventFd) {
      if (FD_ISSET(eventFd, &readFds)) {
        if (is_in_server_map(eventFd)) {
          acceptClient(mainFds, maxFd, eventFd);
        } else {
          handleClient(mainFds, maxFd, eventFd);
        }
      }
    }
  }
}

void SelectMultiplexer::addFd(fd_set &fdSet, int &maxFd, int fd) {
  FD_SET(fd, &fdSet);
  maxFd = std::max(fd, maxFd);
}

void SelectMultiplexer::removeFd(fd_set &fdSet, int &maxFd, int fd) {
  FD_CLR(fd, &fdSet);
  if (maxFd != fd) {
    return;
  }
  maxFd = -1;
  for (int tmp = fd - 1; tmp >= 0; --tmp) {
    if (FD_ISSET(tmp, &fdSet)) {
      maxFd = tmp;
      break;
    }
  }
}

void SelectMultiplexer::addAllServerFdsToFdSet(fd_set &fdSet, int &maxFd) {
  for (std::map<int, Server *>::iterator it = server_map.begin();
       it != server_map.end(); ++it) {
    addFd(fdSet, maxFd, it->first);
  }
}

void SelectMultiplexer::acceptClient(fd_set &fdSet, int &maxFd, int serverFd) {
  struct sockaddr_storage clientAddr;
  socklen_t addrlen = sizeof(clientAddr);
  int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &addrlen);
  if (clientFd == -1) {
    std::cerr << "accept failed\n";
    return;
  }
  std::cout << "New connection on client fd: " << clientFd << "\n";
  try {
    // TODO: 設計変更のため一旦コメントアウト
    // addClientFd(clientFd, getServerFromServerFdMap(serverFd));
    addFd(fdSet, maxFd, clientFd);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    close(clientFd);
  }
}

void SelectMultiplexer::handleClient(fd_set &fdSet, int &maxFd, int clientFd) {
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
    remove_client_fd(clientFd);
    removeFd(fdSet, maxFd, clientFd);
    return;
  }
  try {
    // TODO: 設計変更のため一旦コメントアウト
    // Server *server = getServerFromClientServerMap(clientFd);
    // server->handleHttp(clientFd, buffer, nbytes);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    close(clientFd);
  }
}

SelectMultiplexer::SelectMultiplexer() {}

SelectMultiplexer::SelectMultiplexer(const SelectMultiplexer &other)
    : Multiplexer(other) {
  (void)other;
}

SelectMultiplexer::~SelectMultiplexer() {}

SelectMultiplexer &
SelectMultiplexer::operator=(const SelectMultiplexer &other) {
  (void)other;
  return *this;
}
