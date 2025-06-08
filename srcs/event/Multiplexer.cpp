/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Multiplexer.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/17 19:42:26 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/06/09 00:02:33 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Multiplexer.hpp"
#include "CgiSession.hpp"
#include "Client.hpp"
#include "ClientRegistry.hpp"
#include "ConnectionManager.hpp"
#include "EpollMultiplexer.hpp"
#include "KqueueMultiplexer.hpp"
#include "Logger.hpp"
#include "PollMultiplexer.hpp"
#include "SelectMultiplexer.hpp"
#include "ServerRegistry.hpp"
#include "ZombieRegistry.hpp"
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

Multiplexer *Multiplexer::instance_ = 0;

const int Multiplexer::k_timeout_ms_ = 1000;

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
  if (instance_) {
    delete instance_;
  }
  instance_ = 0;
}

void Multiplexer::set_server_registry(ServerRegistry *registry) {
  server_registry_ = registry;
}

void Multiplexer::set_client_registry(ClientRegistry *registry) {
  client_registry_ = registry;
}

void Multiplexer::set_zombie_registry(ZombieRegistry *registry) {
  zombie_registry_ = registry;
}

void Multiplexer::register_cgi_fds(int stdin_fd, int stdout_fd,
                                   CgiSession *session) {
  client_registry_->add_cgi(stdin_fd, session);
  client_registry_->add_cgi(stdout_fd, session);
}

void Multiplexer::track_zombie(pid_t pid) { zombie_registry_->track(pid); }

void Multiplexer::process_event(int fd, bool readable, bool writable) {
  if (!readable && !writable) {
    return;
  }
  if (server_registry_->has(fd)) {
    if (readable) {
      accept_client(fd);
    }
    return;
  }
  if (client_registry_->has(fd)) {
    if (readable) {
      read_from_client(fd);
    }
    if (writable) {
      write_to_client(fd);
    }
  }
  if (client_registry_->has_cgi(fd)) {
    if (readable) {
      read_from_cgi(fd);
    }
    if (writable) {
      write_to_cgi(fd);
    }
  }
}

void Multiplexer::handle_timeouts() {

  std::set<CgiSession *> timed_out_cgis =
      client_registry_->mark_timed_out_cgis();
  for (std::set<CgiSession *>::iterator it = timed_out_cgis.begin();
       it != timed_out_cgis.end(); ++it) {
    monitor_write((*it)->get_client_fd());
    cleanup_cgi((*it)->get_stdin_fd());
    cleanup_cgi((*it)->get_stdout_fd());
  }

  std::vector<int> timed_out_fds = client_registry_->mark_timed_out_clients();

  for (size_t i = 0; i < timed_out_fds.size(); ++i) {
    logfd(LOG_INFO,
          "[TIMEOUT] monitor_write after timeout fd=", timed_out_fds[i]);
    monitor_write(timed_out_fds[i]);
  }

  std::vector<int> unresponsive_fds =
      client_registry_->detect_unresponsive_clients();

  for (size_t i = 0; i < unresponsive_fds.size(); ++i) {
    logfd(LOG_INFO,
          "[TIMEOUT] forcibly removing unresponsive fd=", unresponsive_fds[i]);
    cleanup_client(unresponsive_fds[i]);
  }
}

void Multiplexer::handle_zombies() {
  zombie_registry_->reap_zombies();
  zombie_registry_->manage_zombies();
}

Multiplexer::Multiplexer() {}

Multiplexer::Multiplexer(const Multiplexer &other) { (void)other; }

Multiplexer::~Multiplexer() {}

void Multiplexer::accept_client(int serverfd) {
  LOG_DEBUG_FUNC_FD(serverfd);
  int clientfd = ConnectionManager::accept_new_connection(serverfd);
  if (clientfd == -1) {
    log(LOG_DEBUG, "Failed to process new connection");
    return;
  }

  Client *client = new Client(clientfd, server_registry_->get_router(serverfd));
  client_registry_->add(clientfd, client);
  monitor_read(clientfd);
  logfd(LOG_DEBUG, "New connection on client socket: ", clientfd);
}

void Multiplexer::read_from_client(int clientfd) {
  LOG_DEBUG_FUNC_FD(clientfd);
  Client *client = client_registry_->get(clientfd);

  switch (client->on_read()) {

  case IO_CONTINUE:
    break;
  case IO_READY_TO_WRITE:
    monitor_write(clientfd);
    break;
  case IO_SHOULD_CLOSE:
    cleanup_client(clientfd);
    break;
  default:
    logfd(LOG_ERROR, "Unhandled I/O Status on client socket: ", clientfd);
    cleanup_client(clientfd);
  }
}

void Multiplexer::write_to_client(int clientfd) {
  LOG_DEBUG_FUNC_FD(clientfd);
  Client *client = client_registry_->get(clientfd);

  switch (client->on_write()) {
  case IO_CONTINUE:
    break;
  case IO_WRITE_COMPLETE:
    unmonitor_write(clientfd);
    break;
  case IO_SHOULD_SHUTDOWN:
    shutdown_write(clientfd);
    break;
  case IO_SHOULD_CLOSE:
    cleanup_client(clientfd);
    break;
  default:
    logfd(LOG_ERROR, "Unhandled I/O Status on client socket: ", clientfd);
    cleanup_client(clientfd);
  }
}

void Multiplexer::shutdown_write(int clientfd) {
  LOG_DEBUG_FUNC_FD(clientfd);
  if (shutdown(clientfd, SHUT_WR) == -1) {
    logfd(LOG_ERROR, "shutdown() failed for fd: ", clientfd);
    cleanup_client(clientfd);
    return;
  }
  unmonitor_write(clientfd);
}

void Multiplexer::cleanup_client(int clientfd) {
  LOG_DEBUG_FUNC_FD(clientfd);
  unmonitor(clientfd);
  client_registry_->remove(clientfd);
}

void Multiplexer::read_from_cgi(int cgi_stdout) {
  LOG_DEBUG_FUNC_FD(cgi_stdout);
  CgiSession *session = client_registry_->get_cgi(cgi_stdout);

  switch (session->on_cgi_read()) {
  case CGI_IO_CONTINUE:
    // Do nothing
    break;
  case CGI_IO_READ_COMPLETE:
    cleanup_cgi(cgi_stdout);
    monitor_write(session->get_client_fd());
    // Client のwrite event から CgiSession::on_done() を呼ぶ
    break;
  case CGI_IO_ERROR:
    cleanup_cgi(cgi_stdout);
    break;
  default:
    logfd(LOG_ERROR, "Unhandled I/O Status on cgi stdout fd: ", cgi_stdout);
    break;
  }
}

void Multiplexer::write_to_cgi(int cgi_stdin) {
  LOG_DEBUG_FUNC_FD(cgi_stdin);
  CgiSession *session = client_registry_->get_cgi(cgi_stdin);

  switch (session->on_cgi_write()) {
  case CGI_IO_CONTINUE:
    // Do nothing
    break;
  case CGI_IO_WRITE_COMPLETE:
    cleanup_cgi(cgi_stdin);
    monitor_pipe_read(session->get_stdout_fd());
    break;
  case CGI_IO_ERROR:
    cleanup_cgi(cgi_stdin);
    cleanup_cgi(session->get_stdout_fd());
    break;
  default:
    logfd(LOG_ERROR, "Unhandled I/O Status on cgi stdin fd: ", cgi_stdin);
    break;
  }
}

void Multiplexer::cleanup_cgi(int cgi_fd) {
  unmonitor(cgi_fd);
  client_registry_->remove_cgi(cgi_fd);
}

Multiplexer &Multiplexer::operator=(const Multiplexer &other) {
  (void)other;
  return *this;
}
