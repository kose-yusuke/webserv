#include "PollMultiplexer.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/socket.h>

Multiplexer &PollMultiplexer::get_instance() {
  if (!Multiplexer::instance) {
    log(LOG_INFO, "PollMultiplexer::get_instance()");
    Multiplexer::instance = new PollMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance;
}

void PollMultiplexer::run() {
  LOG_DEBUG_FUNC();
  static const int max_poll_events = 65536;
  pfds.reserve(get_num_servers());
  initialize_fds();
  if (pfds.empty()) {
    throw std::runtime_error("pollfd empty");
  }

  while (true) {
    if (pfds.size() >= max_poll_events) {
      throw std::runtime_error("poll() fd count exceeds limit");
    }
    errno = 0;
    int nfd = poll(pfds.data(), pfds.size(), 0);
    if (nfd == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("poll() failed");
    }

    PollFdVec tmp = pfds;
    for (size_t i = 0; i < tmp.size(); ++i) {
      process_event(tmp[i].fd, is_readable(tmp[i]), is_writable(tmp[i]));
    }
  }
}

void PollMultiplexer::monitor_read(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  PollFdIt it = find_pollfd(fd);
  if (it != pfds.end()) {
    if (it->events & POLLIN) {
      return;
    }
    logfd(LOG_WARNING, "fd found, adding missing read event (POLLIN): ", fd);
    it->events |= POLLIN;
    return;
  }
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  pfds.push_back(pfd);
}

void PollMultiplexer::monitor_write(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  PollFdIt it = find_pollfd(fd);
  if (it != pfds.end()) {
    if (it->events & POLLOUT) {
      return;
    }
    it->events |= POLLOUT;
    return;
  }
  logfd(LOG_WARNING, "fd not in read monitor. Adding it now: ", fd);
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN | POLLOUT;
  pfds.push_back(pfd);
}

void PollMultiplexer::unmonitor_write(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    logfd(LOG_WARNING, "fd doesn't exist in pfds vector: ", fd);
    return;
  }
  if (!(it->events & POLLOUT)) {
    logfd(LOG_WARNING, "fd is already delisted from write monitor: ", fd);
    return;
  }
  it->events &= ~POLLOUT;
}

void PollMultiplexer::unmonitor(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  for (size_t i = 0; i < pfds.size(); ++i) {
    for (size_t j = i + 1; j < pfds.size(); ++j) {
      if (pfds[i].fd == pfds[j].fd) {
        logfd(LOG_ERROR, "Duplicate fd in pfds: ", pfds[i].fd);
      }
    }
  }

  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    logfd(LOG_WARNING, "fd already erased: ", fd);
    return;
  }
  pfds.erase(it);
}

bool PollMultiplexer::is_readable(struct pollfd pfd) const {
  return (pfd.revents & (POLLIN | POLLHUP | POLLERR)) != 0;
}

bool PollMultiplexer::is_writable(struct pollfd pfd) const {
  return (pfd.revents & POLLOUT) != 0;
}

PollMultiplexer::PollFdIt PollMultiplexer::find_pollfd(int fd) {
  for (PollFdIt it = pfds.begin(); it != pfds.end(); ++it) {
    if (it->fd == fd) {
      return it;
    }
  }
  return pfds.end();
}

PollMultiplexer::PollMultiplexer() : Multiplexer(), pfds() {}

PollMultiplexer::PollMultiplexer(const PollMultiplexer &other)
    : Multiplexer(other) {
  (void)other;
}

PollMultiplexer::~PollMultiplexer() {}

PollMultiplexer &PollMultiplexer::operator=(const PollMultiplexer &other) {
  (void)other;
  return *this;
}
