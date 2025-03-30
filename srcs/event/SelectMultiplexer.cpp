#include "SelectMultiplexer.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

Multiplexer &SelectMultiplexer::get_instance() {
  if (!Multiplexer::instance) {
    log(LOG_INFO, "SelectMultiplexer::get_instance()");
    Multiplexer::instance = new SelectMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance;
}

void SelectMultiplexer::run() {
  LOG_DEBUG_FUNC();
  initialize_fds();

  while (true) {
    active_read_fds = read_fds;
    active_write_fds = write_fds;
    int nfd = select(max_fd + 1, &active_read_fds, &active_write_fds, 0, 0);
    if (nfd == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("select() failed");
    }

    logfd(LOG_DEBUG, "select() returned: ", nfd);

    for (int fd = 0; fd <= max_fd; ++fd) {
      process_event(fd, is_readable(fd), is_writable(fd));
    }
  }
}

void SelectMultiplexer::add_to_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  FD_SET(fd, &read_fds);
  max_fd = std::max(fd, max_fd);
}

void SelectMultiplexer::remove_from_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  FD_CLR(fd, &read_fds);
  if (fd != max_fd) {
    return;
  }
  for (int tmp_fd = fd - 1; tmp_fd >= 0; --tmp_fd) {
    if (FD_ISSET(tmp_fd, &read_fds)) {
      max_fd = tmp_fd;
      break;
    }
  }
}

void SelectMultiplexer::add_to_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  FD_SET(fd, &write_fds);
}

void SelectMultiplexer::remove_from_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  FD_CLR(fd, &write_fds);
}

bool SelectMultiplexer::is_readable(int fd) {
  return FD_ISSET(fd, &active_read_fds);
}

bool SelectMultiplexer::is_writable(int fd) {
  return FD_ISSET(fd, &active_write_fds);
}

SelectMultiplexer::SelectMultiplexer() : Multiplexer(), max_fd(0) {
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&active_read_fds);
  FD_ZERO(&active_write_fds);
}

SelectMultiplexer::SelectMultiplexer(const SelectMultiplexer &other)
    : Multiplexer(other) {}

SelectMultiplexer::~SelectMultiplexer() {}

SelectMultiplexer &
SelectMultiplexer::operator=(const SelectMultiplexer &other) {
  (void)other;
  return *this;
}
