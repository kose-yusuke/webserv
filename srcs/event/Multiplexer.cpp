#include "Multiplexer.hpp"
#include "EpollMultiplexer.hpp"
#include "KqueueMultiplexer.hpp"
#include "PollMultiplexer.hpp"
#include "SelectMultiplexer.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>

void Multiplexer::add_server_fd(int fd, Server *server) {
  server_map[fd] = server;
}

void Multiplexer::remove_server_fd(int fd) { server_map.erase(fd); }

bool Multiplexer::is_in_server_map(int fd) {
  return server_map.find(fd) != server_map.end();
}

Server *Multiplexer::get_server_from_map(int fd) {
  ServerIt it = server_map.find(fd);
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
  ClientIt it = client_map.find(fd);
  if (it == client_map.end()) {
    std::stringstream ss;
    ss << "Client not found for fd: ";
    ss << fd;
    throw std::runtime_error(ss.str());
  }
  return it->second;
}

void Multiplexer::free_all_fds() {
  for (ServerIt it = server_map.begin(); it != server_map.end(); ++it) {
    close(it->first);
    delete it->second;
  }
  server_map.clear();
  for (ClientIt it = client_map.begin(); it != client_map.end(); ++it) {
    close(it->first);
    delete it->second;
  }
  client_map.clear();
}

Multiplexer::Multiplexer() {}

Multiplexer::Multiplexer(const Multiplexer &other) { (void)other; }

Multiplexer::~Multiplexer() { free_all_fds(); }

Multiplexer &Multiplexer::operator=(const Multiplexer &other) {
  (void)other;
  return *this;
}

/*
Multiplexer &init_multiplexer() {
  // TODO: staticのrun()から、get_instance()に修正する;
  // 各派生クラスの修正が済んでから反映
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
*/
