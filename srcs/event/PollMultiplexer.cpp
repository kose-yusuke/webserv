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
    int nfd = poll(pfds.data(), pfds.size(), 0);
    if (nfd == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("poll() failed");
    }

    logfd(LOG_DEBUG, "poll() returned: ", nfd);

    PollFdVec tmp = pfds;
    for (size_t i = 0; i < tmp.size(); ++i) {
      process_event(tmp[i].fd, is_readable(tmp[i]), is_writable(tmp[i]));
    }
  }
}

void PollMultiplexer::add_to_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  PollFdIt it = find_pollfd(fd);
  if (it != pfds.end()) {
    logfd(LOG_WARNING, "fd already registered: ", fd);
    it->events |= POLLIN;
    return;
  }
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  pfds.push_back(pfd);
}

void PollMultiplexer::remove_from_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    logfd(LOG_WARNING, "fd already erased: ", fd);
    return;
  }
  pfds.erase(it);
}

void PollMultiplexer::add_to_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    logfd(LOG_WARNING, "fd not in read monitor. Adding it now: ", fd);
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLOUT;
    pfds.push_back(pfd);
    return;
  }
  if (it->events & POLLOUT) {
    logfd(LOG_WARNING, "fd already registered for write monitor: ", fd);
    return;
  }
  it->events |= POLLOUT;
}

void PollMultiplexer::remove_from_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    logfd(LOG_WARNING, "fd doesn't exist in pfds vector: ", fd);
    return;
  }
  if (!(it->events & POLLOUT)) {
    logfd(LOG_WARNING, "fd is already in write monitor: ", fd);
    return;
  }
  it->events &= ~POLLOUT;
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
