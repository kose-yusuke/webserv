#include "EpollMultiplexer.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include <iostream>

Multiplexer &EpollMultiplexer::get_instance() {
  if (!Multiplexer::instance_) {
    log(LOG_INFO, "EpollMultiplexer::get_instance()");
    Multiplexer::instance_ = new EpollMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance_;
}

void EpollMultiplexer::run() {
  LOG_DEBUG_FUNC();
  static const int max_epoll_events = 3599293;
  int size = 16; // num of fd; expect to monitor; not upper limit

  std::vector<struct epoll_event> evlist;

  while (true) {
    if (size > max_epoll_events) {
      throw std::runtime_error("epoll event list exceeds limit");
    }
    evlist.resize(size);
    handle_timeouts();
    errno = 0;
    int nfd = epoll_wait(epfd_, evlist.data(), evlist.size(), k_timeout_ms_);
    if (nfd == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("epoll_wait() failed");
    }

    for (int i = 0; i < nfd; ++i) {
      process_event(evlist[i].data.fd, is_readable(evlist[i]),
                    is_writable(evlist[i]));
    }
    if (nfd == size) {
      size *= 2;
    }
  }
}

void EpollMultiplexer::monitor_read(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.events = is_in_write_fds(fd) ? EPOLLIN | EPOLLOUT : EPOLLIN;
  ev.data.fd = fd;

  if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
    if (errno != EEXIST) {
      logfd(LOG_ERROR, "failed to list fd in read monitor: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd already under monitor: ", fd);
    if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
      logfd(LOG_ERROR, "failed to list fd in read monitor: ", fd);
      return;
    }
  }
  read_fds_.insert(fd);
}

void EpollMultiplexer::monitor_write(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN | EPOLLOUT;

  if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {

    if (errno != ENOENT) {
      logfd(LOG_ERROR, "failed to list fd in write monitor: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd not in read monitor. Adding it now: ", fd);
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
      logfd(LOG_ERROR, "failed to list fd in both monitor: ", fd);
      return;
    }
    read_fds_.insert(fd);
  }
  write_fds_.insert(fd);
}

void EpollMultiplexer::unmonitor_write(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN;

  if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {

    if (errno != ENOENT) {
      logfd(LOG_ERROR, "failed to delist fd in write monitor: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd not in read monitor. Adding it now: ", fd);
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
      logfd(LOG_ERROR, "failed to list fd in read monitor: ", fd);
      return;
    }
    read_fds_.insert(fd);
  }
  write_fds_.erase(fd);
}

void EpollMultiplexer::unmonitor(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.events = 0;
  ev.data.fd = fd;

  if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, &ev) == -1) {
    if (errno != ENOENT) {
      logfd(LOG_ERROR, "failed to delete fd: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd already erased: ", fd);
  }
  read_fds_.erase(fd);
  write_fds_.erase(fd);
}

bool EpollMultiplexer::is_readable(struct epoll_event &ev) const {
  return ev.events & EPOLLIN;
}

bool EpollMultiplexer::is_writable(struct epoll_event &ev) const {
  return ev.events & EPOLLOUT;
}

bool EpollMultiplexer::is_in_read_fds(int fd) const {
  return read_fds_.find(fd) != read_fds_.end();
}

bool EpollMultiplexer::is_in_write_fds(int fd) const {
  return write_fds_.find(fd) != write_fds_.end();
}

EpollMultiplexer::EpollMultiplexer() {
  epfd_ = epoll_create(16);
  if (epfd_ == -1) {
    throw std::runtime_error("epoll_create");
  }
}

EpollMultiplexer::EpollMultiplexer(const EpollMultiplexer &other)
    : Multiplexer(other) {}

EpollMultiplexer::~EpollMultiplexer() {}

EpollMultiplexer &EpollMultiplexer::operator=(const EpollMultiplexer &other) {
  (void)other;
  return *this;
}
