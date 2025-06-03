#include "ClientRegistry.hpp"
#include "CgiSession.hpp"
#include "Client.hpp"
#include "Logger.hpp"

ClientRegistry::ClientRegistry() {}

ClientRegistry::~ClientRegistry() {
  for (CgiIt cgi_it = cgis_.begin(); cgi_it != cgis_.end(); ++cgi_it) {
    if (close(cgi_it->first) == -1) {
      logfd(LOG_ERROR, "Failed to close cgi fd: ", cgi_it->first);
    }
    // Note: CgiSession*は、ClientRegistryではなくClientが所有権を持つ
    // CgiSession::pid_ は、CgiSessionのデストラクタがkillする
    // stdin_fd_, stdout_fd_ のcloseのみここで行う
  }
  cgis_.clear();

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
      it->second->on_timeout();
      timed_out_clients.push_back(it->first);
    }
  }
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
  return unresponsive_clients;
}

void ClientRegistry::add_cgi(int fd, CgiSession *session) {
  if (has_cgi(fd)) {
    logfd(LOG_ERROR, "Duplicate cgi fd: ", fd);
    return;
  }
  cgis_[fd] = session;
}

// ClientRegistyがfdの所有権を持つ. ClientがCgiSessionの所有権を持つ
void ClientRegistry::remove_cgi(int fd) {
  CgiIt it = cgis_.find(fd);
  if (it == cgis_.end()) {
    return;
  }
  if (close(it->first) == -1) {
    logfd(LOG_ERROR, "Failed to close cgi fd: ", fd);
  }
  cgis_.erase(it);
}

CgiSession *ClientRegistry::get_cgi(int fd) const {
  ConstCgiIt it = cgis_.find(fd);
  if (it == cgis_.end()) {
    logfd(LOG_ERROR, "Failed to find cgi fd: ", fd);
    return NULL;
  }
  return it->second;
}

bool ClientRegistry::has_cgi(int fd) const {
  return cgis_.find(fd) != cgis_.end();
}

std::set<CgiSession *> ClientRegistry::mark_timed_out_cgis() {
  time_t now = time(NULL);
  std::set<CgiSession *> timed_out_cgis;

  for (CgiIt it = cgis_.begin(); it != cgis_.end(); ++it) {
    if (it->second->is_cgi_timeout(now)) {
      it->second->on_cgi_timeout(); // stateの更新; pidのkill
      timed_out_cgis.insert(it->second);
    }
  }
  return timed_out_cgis;
}

ClientRegistry &ClientRegistry::operator=(const ClientRegistry &other) {
  (void)other;
  return *this;
}
