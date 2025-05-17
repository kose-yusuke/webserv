#include "ClientRegistry.hpp"
#include "Client.hpp"
#include "Logger.hpp"

ClientRegistry::ClientRegistry() {}

ClientRegistry::~ClientRegistry() {
  for (ClientIt it = clients_.begin(); it != clients_.end(); ++it) {
    if (it->first != -1 && close(it->first) == -1) {
      logfd(LOG_ERROR, "Failed to close client fd: ", it->first);
    }
    delete it->second;
  }
  clients_.clear();
}

void ClientRegistry::add(int fd, Client *client) {
  if (has(fd)) {
    logfd(LOG_ERROR, "Duplicate client fd: ", fd);
    return;
  }
  clients_[fd] = client;
}

// Note: ClientRegistryがfdの所有権を持つため、ここでclose(fd)
void ClientRegistry::remove(int fd) {
  ClientIt it = clients_.find(fd);
  if (it == clients_.end()) {
    logfd(LOG_ERROR, "Client not in registry fd: ", fd);
    return;
  }
  if (close(it->first) == -1) {
    logfd(LOG_ERROR, "Failed to close client fd: ", fd);
  }
  delete it->second;
  clients_.erase(fd);
}

Client *ClientRegistry::get(int fd) const {
  ConstClientIt it = clients_.find(fd);
  if (it == clients_.end()) {
    logfd(LOG_ERROR, "Failed to find client fd: ", fd);
    return NULL;
  }
  return it->second;
}

bool ClientRegistry::has(int fd) const {
  return clients_.find(fd) != clients_.end();
}

size_t ClientRegistry::size() const { return clients_.size(); }

std::vector<int> ClientRegistry::mark_timed_out_clients() {
  time_t now = time(NULL);
  std::vector<int> timed_out_clients;
  timed_out_clients.reserve(clients_.size());

  for (ClientIt it = clients_.begin(); it != clients_.end(); ++it) {
    if (it->second->is_timeout(now)) {
      it->second->on_timeout(); // stateの更新と、timeout responseの準備
      timed_out_clients.push_back(it->first);
    }
  }
  // multiplexerにmonitor_write()を頼みたいfdsを渡す
  return timed_out_clients;
}

std::vector<int> ClientRegistry::detect_unresponsive_clients() const {
  time_t now = time(NULL);
  std::vector<int> unresponsive_clients;
  unresponsive_clients.reserve(clients_.size());

  for (ConstClientIt it = clients_.begin(); it != clients_.end(); ++it) {
    if (it->second->is_unresponsive(now)) {
      unresponsive_clients.push_back(it->first);
    }
  }
  // multiplexerにforce closeさせたいfdsを渡す
  return unresponsive_clients;
}

ClientRegistry &ClientRegistry::operator=(const ClientRegistry &other) {
  (void)other;
  return *this;
}
