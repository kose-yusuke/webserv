#include "Multiplexer.hpp"
#include "Epoller.hpp"
#include "KqueueMultiplexer.hpp"
#include "Poller.hpp"
#include "SelectMultiplexer.hpp"

/**
 * OSにあわせたMultiplexerの切り替え
 *  */
void Multiplexer::run() {
  if (serverFdMap_.empty()) {
    throw std::runtime_error("No listening sockets available");
  }
#ifdef __linux__
  Epoller::run();
#elif defined(__APPLE__) || defined(__MACH__)
  KqueueMultiplexer::run();
#elif defined(_WIN32) || defined(_WIN64)
  SelectMultiplexer::run();
#else
  Poller::run();
#endif
}

std::map<int, Server *> Multiplexer::serverFdMap_;
std::map<int, Server *> Multiplexer::clientServerMap_;

void Multiplexer::addServerFd(int serverFd, Server *server) {
  serverFdMap_[serverFd] = server;
}

void Multiplexer::removeServerFd(int serverFd) { serverFdMap_.erase(serverFd); }

bool Multiplexer::isInServerFdMap(int serverFd) {
  return serverFdMap_.find(serverFd) != serverFdMap_.end();
}

Server *Multiplexer::getServerFromServerFdMap(int serverFd) {
  std::map<int, Server *>::iterator it = serverFdMap_.find(serverFd);
  return (it != serverFdMap_.end()) ? it->second : NULL;
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
  return (it != clientServerMap_.end()) ? it->second : NULL;
}

Multiplexer::Multiplexer() {}

Multiplexer::~Multiplexer() {}

Multiplexer::Multiplexer(const Multiplexer &other) { (void)other; }

Multiplexer &Multiplexer::operator=(const Multiplexer &other) {
  (void)other;
  return *this;
}
