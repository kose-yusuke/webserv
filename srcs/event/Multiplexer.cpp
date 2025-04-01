#include "Multiplexer.hpp"
#include "Client.hpp"
#include "EpollMultiplexer.hpp"
#include "KqueueMultiplexer.hpp"
#include "PollMultiplexer.hpp"
#include "SelectMultiplexer.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <unistd.h>

Multiplexer *Multiplexer::instance = 0;

Multiplexer &Multiplexer::get_instance() {
#if defined(__linux__)
  return EpollMultiplexer::get_instance();
#elif defined(__APPLE__) || defined(__MACH__)
  return KqueueMultiplexer::get_instance();
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
  return KqueueMultiplexer::get_instance();
#elif defined(HAS_POLL)
  return PollMultiplexer::get_instance();
#else
  return SelectMultiplexer::get_instance();
#endif
}

void Multiplexer::delete_instance() {
  delete instance;
  instance = 0;
}

void Multiplexer::add_to_server_map(int fd, Server *server) {
  if (is_in_server_map(fd)) {
    std::ostringstream oss;
    oss << "Duplicate server fd: " << fd;
    throw std::runtime_error(oss.str());
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

bool Multiplexer::is_in_server_map(int fd) const {
  return server_map.find(fd) != server_map.end();
}

Server *Multiplexer::get_server_from_map(int fd) const {
  ConstServerIt it = server_map.find(fd);
  if (it == server_map.end()) {
    std::ostringstream oss;
    oss << "Server not found for fd: " << fd;
    throw std::runtime_error(oss.str());
  }
  return it->second;
}

size_t Multiplexer::get_num_servers() const { return server_map.size(); }

void Multiplexer::add_to_client_map(int fd, Client *client) {
  if (is_in_client_map(fd)) {
    std::ostringstream oss;
    oss << "Duplicate client fd: " << fd;
    throw std::runtime_error(oss.str());
  }
  client_map[fd] = client;
}

void Multiplexer::remove_from_client_map(int fd) {
  ClientIt it = client_map.find(fd);
  if (it != client_map.end()) {
    if (close(it->first) == -1) {
      logfd(LOG_ERROR, "Failed to close client fd: ", fd);
    } else {
      logfd(LOG_DEBUG, "Closed client fd: ", fd);
    }
    delete it->second;
    client_map.erase(fd);
  }
}

bool Multiplexer::is_in_client_map(int fd) const {
  return client_map.find(fd) != client_map.end();
}

Client *Multiplexer::get_client_from_map(int fd) const {
  ConstClientIt it = client_map.find(fd);
  if (it == client_map.end()) {
    std::ostringstream oss;
    oss << "Client not found for fd: " << fd;
    throw std::runtime_error(oss.str());
  }
  return it->second;
}

size_t Multiplexer::get_num_clients() const { return client_map.size(); }

void Multiplexer::initialize_fds() {
  LOG_DEBUG_FUNC();
  if (server_map.empty()) {
    throw std::runtime_error("Error: No servers available.");
  }
  for (ServerIt it = server_map.begin(); it != server_map.end(); ++it) {
    monitor_read(it->first);
  }
}

void Multiplexer::process_event(int fd, bool readable, bool writable) {
  if (!readable && !writable) {
    return;
  }
  if (is_in_server_map(fd)) {
    if (readable) {
      accept_client(fd);
    }
    return;
  }
  if (is_in_client_map(fd) && readable) {
    read_from_client(fd);
  }
  if (is_in_client_map(fd) && writable) {
    write_to_client(fd);
  }
}

void Multiplexer::free_all_fds() {
  for (ServerIt it = server_map.begin(); it != server_map.end(); ++it) {
    close(it->first);
    // delete it->second;
    // 複数のfd（port）が同じserverに結びつくので、ここでserverインスタンスを削除できない
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

void Multiplexer::accept_client(int server_fd) {
  LOG_DEBUG_FUNC_FD(server_fd);
  struct sockaddr_storage client_addr;
  socklen_t addrlen = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
  if (client_fd == -1) {
    log(LOG_ERROR, "accept failed");
    return;
  }
  if (fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL) | O_NONBLOCK) == -1) {
    logfd(LOG_ERROR, "Failed to set O_NONBLOCK: ", client_fd);
    close(client_fd);
    return;
  }
  int optval = 1;
  if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    logfd(LOG_ERROR, "Failed to set SO_REUSEADDR on client fd: ", client_fd);
    close(client_fd);
    return;
  }

  try {
    add_to_client_map(client_fd, new Client(client_fd, server_fd));
  } catch (const std::exception &e) {
    logfd(LOG_ERROR, e.what(), client_fd);
    close(client_fd);
    return;
  }
  monitor_read(client_fd);
  logfd(LOG_DEBUG, "New connection on client fd: ", client_fd);
}

void Multiplexer::read_from_client(int client_fd) {
  LOG_DEBUG_FUNC_FD(client_fd);
  Client *client = get_client_from_map(client_fd);

  switch (client->on_read()) {
  case IO_SUCCESS:
    monitor_write(client_fd);
    break;
  case IO_CONTINUE:
    break;
  case IO_CLOSED:
    remove_client(client_fd);
    break;
  case IO_ERROR:
    remove_client(client_fd);
    break;
  default:
    logfd(LOG_ERROR, "Unhandled IOStatus: ", client_fd);
    remove_client(client_fd);
  }
}

void Multiplexer::write_to_client(int client_fd) {
  LOG_DEBUG_FUNC_FD(client_fd);
  Client *client = get_client_from_map(client_fd);

  switch (client->on_write()) {
  case IO_SUCCESS:
    unmonitor_write(client_fd);
    break;
  case IO_CONTINUE:
    break;
  case IO_CLOSED:
    unmonitor_write(client_fd);
    break;
  case IO_ERROR:
    remove_client(client_fd);
    break;
  default:
    logfd(LOG_ERROR, "Unhandled IOStatus: ", client_fd);
    remove_client(client_fd);
  }
}

void Multiplexer::remove_client(int client_fd) {
  LOG_DEBUG_FUNC_FD(client_fd);
  unmonitor(client_fd);
  remove_from_client_map(client_fd); // close, delete もこの中
}

Multiplexer &Multiplexer::operator=(const Multiplexer &other) {
  (void)other;
  return *this;
}
