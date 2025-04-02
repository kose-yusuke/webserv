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
  pfds.reserve(get_num_servers());
  initialize_fds();
  if (pfds.empty()) {
    throw std::runtime_error("pollfd empty");
  }

  while (true) {
    if (pfds.size() > max_poll_events) {
      throw std::runtime_error("poll event list exceeds limit");
    }
    errno = 0;
    int nfd = poll(pfds.data(), pfds.size(), 10);
    if (nfd == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("poll() failed");
    }
    for (size_t i = 0; i < pfds.size() && nfd > 0; ++i) {
      if (pfds[i].fd == -1 || pfds[i].revents == 0) {
        continue;
      }
      process_event(pfds[i].fd, is_readable(pfds[i]), is_writable(pfds[i]));
      pfds[i].revents = 0;
      nfd--;
    }
  }
}

void PollMultiplexer::monitor_read(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  if (fd >= max_poll_events) {
    logfd(LOG_ERROR, "fd exceeds poll limit (max_poll_events): ", fd);
    return;
  }

  if (static_cast<size_t>(fd) >= pfds.size()) {
    size_t old_size = pfds.size();
    pfds.resize(fd + 1);
    sanitize_resized_pfds(old_size);
  }

  if (pfds[fd].fd == fd && (pfds[fd].events & POLLIN)) {
    logfd(LOG_DEBUG, "fd is already monitored for read: ", fd);
    return;
  }

  if (pfds[fd].fd != -1 && pfds[fd].fd != fd) {
    logfd(LOG_WARNING, "Unexpected state: pfds[fd].fd != fd: ", fd);
  }

  pfds[fd].fd = fd;
  pfds[fd].events |= POLLIN;
}

void PollMultiplexer::monitor_write(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  if (static_cast<size_t>(fd) >= pfds.size()) {
    size_t old_size = pfds.size();
    pfds.resize(fd + 1);
    sanitize_resized_pfds(old_size);
  }

  if (pfds[fd].fd == -1 || pfds[fd].fd != fd) {
    logfd(LOG_WARNING, "Unexpected state: pfds[fd].fd != fd: ", fd);
    pfds[fd].fd = fd;
  }

  pfds[fd].events = POLLIN | POLLOUT;
}

void PollMultiplexer::unmonitor_write(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  if (fd >= max_poll_events) {
    logfd(LOG_ERROR, "fd exceeds poll limit (max_poll_events): ", fd);
    return;
  }

  if (static_cast<size_t>(fd) >= pfds.size()) {
    size_t old_size = pfds.size();
    pfds.resize(fd + 1);
    sanitize_resized_pfds(old_size);
  }

  if (pfds[fd].fd == -1) {
    logfd(LOG_WARNING, "fd is already fully unmonitored: ", fd);
    return;
  }

  if (pfds[fd].fd != fd) {
    logfd(LOG_WARNING, "Unexpected state: pfds[fd].fd != fd: ", fd);
    pfds[fd].fd = fd;
  }

  if (!(pfds[fd].events & POLLOUT)) {
    logfd(LOG_WARNING, "fd is not monitored for write (already removed): ", fd);
    return;
  }
  pfds[fd].events &= ~POLLOUT;
}

void PollMultiplexer::unmonitor(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  if (static_cast<size_t>(fd) >= pfds.size()) {
    size_t old_size = pfds.size();
    pfds.resize(fd + 1);
    sanitize_resized_pfds(old_size);
  }

  if (pfds[fd].fd == -1) {
    logfd(LOG_WARNING, "fd is already fully unmonitored: ", fd);
    return;
  }

  if (pfds[fd].fd != fd) {
    logfd(LOG_WARNING, "Unexpected state: pfds[fd].fd != fd: ", fd);
  }

  pfds[fd].fd = -1;
  pfds[fd].events = 0;
  pfds[fd].revents = 0;
}

bool PollMultiplexer::is_readable(struct pollfd pfd) const {
  return (pfd.revents & (POLLIN | POLLHUP | POLLERR)) != 0;
}

bool PollMultiplexer::is_writable(struct pollfd pfd) const {
  return (pfd.revents & POLLOUT) != 0;
}

void PollMultiplexer::sanitize_resized_pfds(size_t old_size) {
  for (size_t i = old_size; i < pfds.size(); ++i) {
    if (pfds[i].fd == 0) {
      pfds[i].fd = -1;
    }
  }
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
