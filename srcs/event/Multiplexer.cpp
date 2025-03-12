#include "Multiplexer.hpp"
#include "Client.hpp"
#include "EpollMultiplexer.hpp"
#include "KqueueMultiplexer.hpp"
#include "PollMultiplexer.hpp"
#include "SelectMultiplexer.hpp"
#include "Server.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <unistd.h>

Multiplexer *Multiplexer::instance = 0;

Multiplexer &Multiplexer::get_instance() {
  if (!instance) {
    // #if defined(__linux__)
    //     instance = new EpollMultiplexer();
    // #elif defined(__APPLE__) || defined(__MACH__)
    //     instance = new SelectMultiplexer();
    // #elif defined(__FreeBSD__) || defined(__OpenBSD__)
    //     instance = new KqueueMultiplexer();
    // #elif defined(HAS_POLL)
    //     instance = new Poller();
    // #else
    //     instance = new SelectMultiplexer();
    // #endif
    instance = new SelectMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *instance;
}

void Multiplexer::delete_instance() {
  delete instance;
  instance = 0;
}

void Multiplexer::add_to_server_map(int fd, Server *server) {
  if (is_in_server_map(fd)) {
    std::stringstream ss;
    ss << "Duplicate server fd: " << fd;
    throw std::runtime_error(ss.str());
  }
  server_map[fd] = server;
}

void Multiplexer::remove_from_server_map(int fd) {
  ServerIt it = server_map.find(fd);
  if (it != server_map.end()) {
    close(it->first);
    delete it->second;
    server_map.erase(it);
  }
}

bool Multiplexer::is_in_server_map(int fd) {
  return server_map.find(fd) != server_map.end();
}

Server *Multiplexer::get_server_from_map(int fd) {
  ServerIt it = server_map.find(fd);
  if (it == server_map.end()) {
    std::stringstream ss;
    ss << "Server not found for fd: " << fd;
    throw std::runtime_error(ss.str());
  }
  return it->second;
}

void Multiplexer::add_to_client_map(int fd, Client *client) {
  if (is_in_client_map(fd)) {
    std::stringstream ss;
    ss << "Duplicate client fd: " << fd;
    throw std::runtime_error(ss.str());
  }
  client_map[fd] = client;
}

void Multiplexer::remove_from_client_map(int fd) {
  ClientIt it = client_map.find(fd);
  if (it != client_map.end()) {
    close(it->first);
    delete it->second;
    client_map.erase(fd);
  }
}

bool Multiplexer::is_in_client_map(int fd) {
  return client_map.find(fd) != client_map.end();
}

Client *Multiplexer::get_client_from_map(int fd) {
  ClientIt it = client_map.find(fd);
  if (it == client_map.end()) {
    std::stringstream ss;
    ss << "Client not found for fd: " << fd;
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
