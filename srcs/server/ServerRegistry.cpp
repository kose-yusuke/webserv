#include "ServerRegistry.hpp"
#include "Logger.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "SocketBuilder.hpp"
#include "VirtualHostRouter.hpp"
#include <iostream>
#include <unistd.h>

ServerRegistry::ServerRegistry() {}

ServerRegistry::~ServerRegistry() {
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].fd != -1 && close(entries[i].fd) == -1) {
      logfd(LOG_ERROR, "Failed to close server fd: ", entries[i].fd);
    }
    delete entries[i].virtual_host_router;
  }
}

// XXX: listen が必ず最低でも1つあることは、config parse時にチェックする
// XXX: 重複についての検証はどこでするか確認する
void ServerRegistry::add(const Server &s) {
  std::vector<std::string> ip_ports = s.get_config().find("listen")->second;
  std::string ip, port;

  for (size_t i = 0; i < ip_ports.size(); ++i) {

    size_t colon_pos = ip_ports[i].find(':');
    if (colon_pos == std::string::npos) {
      ip = "0.0.0.0";
      port = ip_ports[i];
    } else {
      ip = ip_ports[i].substr(0, colon_pos);
      port = ip_ports[i].substr(colon_pos + 1);
    }

    size_t j;
    for (j = 0; j < entries.size(); ++j) {
      if (ip == entries[j].ip && port == entries[j].port) {
        entries[j].virtual_host_router->add(new Server(s));
        break;
      }
    }
    if (j == entries.size()) {
      create_new_listen_target(ip, port, s);
    }
  }
}

void ServerRegistry::remove(int fd) {
  for (ListenEntryIt it = entries.begin(); it != entries.end(); ++it) {
    if (it->fd == fd) {
      if (close(it->fd) == -1) {
        logfd(LOG_ERROR, "Failed to close server fd: ", it->fd);
      }
      delete it->virtual_host_router;
      entries.erase(it);
      return;
    }
  }
}

bool ServerRegistry::has(int fd) const {
  for (size_t i = 0; i < entries.size(); ++i) {
    if (entries[i].fd == fd) {
      return true;
    }
  }
  return false;
}

size_t ServerRegistry::size() const { return entries.size(); }

void ServerRegistry::initialize() {
  for (size_t i = 0; i < entries.size(); ++i) {
    int fd = SocketBuilder::create_socket(entries[i].ip, entries[i].port);
    entries[i].fd = fd;
    Multiplexer::get_instance().monitor_read(fd);
  }
}

const VirtualHostRouter *ServerRegistry::get_router(int fd) const {
  for (ConstListenEntryIt it = entries.begin(); it != entries.end(); ++it) {
    if (it->fd == fd) {
      return it->virtual_host_router;
    }
  }
  return NULL;
}

void ServerRegistry::create_new_listen_target(const std::string &ip,
                                              const std::string &port,
                                              const Server &s) {
  struct ListenEntry entry;
  entry.fd = -1;
  entry.ip = ip;
  entry.port = port;
  entry.virtual_host_router = new VirtualHostRouter;
  entry.virtual_host_router->add(new Server(s));
  entries.push_back(entry);
}

ServerRegistry &ServerRegistry::operator=(const ServerRegistry &other) {
  (void)other;
  return *this;
}
