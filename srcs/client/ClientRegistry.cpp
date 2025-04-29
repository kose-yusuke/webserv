#include "ClientRegistry.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include <cstddef>

ClientRegistry::ClientRegistry() {}

ClientRegistry::~ClientRegistry() {
  for (ClientIt it = clients.begin(); it != clients.end(); ++it) {
    if (it->first != -1 && close(it->first) == -1) {
      logfd(LOG_ERROR, "Failed to close client fd: ", it->first);
    }
    delete it->second;
  }
  clients.clear();
}

void ClientRegistry::add(int fd, Client *client) {
  if (has(fd)) {
    logfd(LOG_ERROR, "Duplicate client fd: ", fd);
    return;
  }
  clients[fd] = client;
}

// Note: ClientRegistryがfdの所有権を持つため、ここでclose(fd)
void ClientRegistry::remove(int fd) {
  ClientIt it = clients.find(fd);
  if (it == clients.end()) {
    logfd(LOG_ERROR, "Client not in registry fd: ", fd);
    return;
  }
  if (close(it->first) == -1) {
    logfd(LOG_ERROR, "Failed to close client fd: ", fd);
  }
  delete it->second;
  clients.erase(fd);
}

Client *ClientRegistry::get(int fd) const {
  ConstClientIt it = clients.find(fd);
  if (it == clients.end()) {
    logfd(LOG_ERROR, "Failed to find client fd: ", fd);
    return NULL;
  }
  return it->second;
}

bool ClientRegistry::has(int fd) const {
  return clients.find(fd) != clients.end();
}

size_t ClientRegistry::size() const { return clients.size(); }

ClientRegistry &ClientRegistry::operator=(const ClientRegistry &other) {
  (void)other;
  return *this;
}
