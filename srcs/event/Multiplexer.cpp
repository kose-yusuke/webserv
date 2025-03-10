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
  if (server_map.empty()) {
    throw std::runtime_error("No listening sockets available");
  }
  SelectMultiplexer::run();
  return;
#if defined(__linux__)
  EpollMultiplexer::run();
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

void Multiplexer::add_server_fd(int fd, Server *server) {
  server_map[fd] = server;
}

void Multiplexer::close_all_fds() {
  for (std::map<int, Server *>::iterator it = server_map.begin();
       it != server_map.end(); ++it) {
    close(it->first);
  }
  for (std::map<int, Client *>::iterator it = client_map.begin();
       it != client_map.end(); ++it) {
    close(it->first);
  }
}

std::map<int, Server *> Multiplexer::server_map;
std::map<int, Client *> Multiplexer::client_map;

void Multiplexer::remove_server_fd(int fd) { server_map.erase(fd); }

bool Multiplexer::is_in_server_map(int fd) {
  return server_map.find(fd) != server_map.end();
}

Server *Multiplexer::get_server_from_map(int fd) {
  std::map<int, Server *>::iterator it = server_map.find(fd);
  if (it == server_map.end()) {

    std::stringstream ss;
    ss << "Server not found for fd: ";
    ss << fd;
    throw std::runtime_error(ss.str());
  }
  return it->second;
}

void Multiplexer::add_client_fd(int fd, Client *client) {
  client_map[fd] = client;
}

void Multiplexer::remove_client_fd(int fd) { client_map.erase(fd); }

bool Multiplexer::is_in_client_map(int fd) {
  return client_map.find(fd) != client_map.end();
}

Client *Multiplexer::get_client_from_map(int fd) {
  std::map<int, Client *>::iterator it = client_map.find(fd);
  if (it == client_map.end()) {
    std::stringstream ss;
    ss << "Client not found for fd: ";
    ss << fd;
    throw std::runtime_error(ss.str());
  }
  return it->second;
}

Multiplexer::Multiplexer() {}

Multiplexer::Multiplexer(const Multiplexer &other) { (void)other; }

Multiplexer::~Multiplexer() {}

Multiplexer &Multiplexer::operator=(const Multiplexer &other) {
  (void)other;
  return *this;
}
