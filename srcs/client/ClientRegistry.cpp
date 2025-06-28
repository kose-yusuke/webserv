#include "ClientRegistry.hpp"
#include "Client.hpp"
#include "Logger.hpp"

ClientRegistry::ClientRegistry() {}

ClientRegistry::~ClientRegistry() {
  for (ClientIt it = fd_to_clients_.begin(); it != fd_to_clients_.end(); ++it) {
    delete it->second;
  }
  fd_to_clients_.clear();
}

void ClientRegistry::add(int fd, Client *client) {
  if (has(fd)) {
    logfd(LOG_ERROR, "Duplicate client fd: ", fd);
    return;
  }
  fd_to_clients_[fd] = client;
}

void ClientRegistry::remove(int fd) {
  ClientIt it = fd_to_clients_.find(fd);
  if (it == fd_to_clients_.end()) {
    logfd(LOG_ERROR, "Client not in registry fd: ", fd);
    return;
  }
  delete it->second;
  fd_to_clients_.erase(fd);
}

Client *ClientRegistry::get(int fd) const {
  ConstClientIt it = fd_to_clients_.find(fd);
  if (it == fd_to_clients_.end()) {
    logfd(LOG_ERROR, "Failed to find client fd: ", fd);
    return NULL;
  }
  return it->second;
}

bool ClientRegistry::has(int fd) const {
  return fd_to_clients_.find(fd) != fd_to_clients_.end();
}

std::vector<int> ClientRegistry::mark_timed_out_clients() {
  time_t now = time(NULL);
  std::vector<int> timed_out_clients;
  timed_out_clients.reserve(fd_to_clients_.size());

  for (ClientIt it = fd_to_clients_.begin(); it != fd_to_clients_.end(); ++it) {
    if (it->second->is_timeout(now)) {
      it->second->on_timeout();
      timed_out_clients.push_back(it->first);
    }
  }
  return timed_out_clients;
}

std::vector<int> ClientRegistry::detect_unresponsive_clients() const {
  time_t now = time(NULL);
  std::vector<int> unresponsive_clients;
  unresponsive_clients.reserve(fd_to_clients_.size());

  for (ConstClientIt it = fd_to_clients_.begin(); it != fd_to_clients_.end();
       ++it) {
    if (it->second->is_unresponsive(now)) {
      unresponsive_clients.push_back(it->first);
    }
  }
  return unresponsive_clients;
}

ClientRegistry &ClientRegistry::operator=(const ClientRegistry &other) {
  (void)other;
  return *this;
}
