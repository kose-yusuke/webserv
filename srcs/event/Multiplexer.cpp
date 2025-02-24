#include "Multiplexer.hpp"
#include "EpollMultiplexer.hpp"
#include "KqueueMultiplexer.hpp"
#include "PollMultiplexer.hpp"
#include "SelectMultiplexer.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>

void Multiplexer::run() {
  std::cout << "Multiplexer::run called\n";
  if (serverFdMap_.empty()) {
    throw std::runtime_error("No listening sockets available");
  }
#if defined(__linux__)
  Epoller::run();
#elif defined(__APPLE__) || defined(__MACH__)
  KqueueMultiplexer::run();
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
  KqueueMultiplexer::run();
#elif defined(HAS_POLL)
  Poller::run();
#else
  SelectMultiplexer::run();
#endif
}

void Multiplexer::addServerFd(int serverFd, Server *server) {
  serverFdMap_[serverFd] = server;
}

void Multiplexer::closeAllFds() {
  for (std::map<int, Server *>::iterator it = serverFdMap_.begin();
       it != serverFdMap_.end(); ++it) {
    close(it->first);
  }
  for (std::map<int, Server *>::iterator it = clientServerMap_.begin();
       it != clientServerMap_.end(); ++it) {
    close(it->first);
  }
}

std::map<int, Server *> Multiplexer::serverFdMap_;
std::map<int, Server *> Multiplexer::clientServerMap_;

void Multiplexer::removeServerFd(int serverFd) { serverFdMap_.erase(serverFd); }

bool Multiplexer::isInServerFdMap(int serverFd) {
  return serverFdMap_.find(serverFd) != serverFdMap_.end();
}

Server *Multiplexer::getServerFromServerFdMap(int serverFd) {
  std::map<int, Server *>::iterator it = serverFdMap_.find(serverFd);
  if (it == serverFdMap_.end()) {

    std::stringstream ss;
    ss << "Server not found for fd: ";
    ss << serverFd;
    throw std::runtime_error(ss.str());
  }
  return it->second;
}

void Multiplexer::addClientFd(int clientFd, Server *server) {
  clientServerMap_[clientFd] = server;
}

void Multiplexer::removeClientFd(int clientFd) {
  clientServerMap_.erase(clientFd);
}

bool Multiplexer::isInClientServerMap(int clientFd) {
  return clientServerMap_.find(clientFd) != clientServerMap_.end();
}

Server *Multiplexer::getServerFromClientServerMap(int clientFd) {
  std::map<int, Server *>::iterator it = clientServerMap_.find(clientFd);
  if (it == clientServerMap_.end()) {
    std::stringstream ss;
    ss << "Server not found for fd: ";
    ss << clientFd;
    throw std::runtime_error(ss.str());
  }
  return it->second;
}

Multiplexer::Multiplexer() {}

Multiplexer::~Multiplexer() {}

Multiplexer::Multiplexer(const Multiplexer &other) { (void)other; }

Multiplexer &Multiplexer::operator=(const Multiplexer &other) {
  (void)other;
  return *this;
}
