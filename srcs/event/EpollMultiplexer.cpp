#include "EpollMultiplexer.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <iostream>

Multiplexer &EpollMultiplexer::get_instance() {
  if (!Multiplexer::instance) {
    log(LOG_INFO, "EpollMultiplexer::get_instance()");
    Multiplexer::instance = new EpollMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance;
}

void EpollMultiplexer::run() {
  LOG_DEBUG_FUNC();

  static const int max_user_watches = 3599293;
  int size = 16; // num of fd; expect to monitor; not upper limit
  epfd = epoll_create(size);
  if (epfd == -1) {
    throw std::runtime_error("epoll_create");
  }
  std::vector<struct epoll_event> evlist;
  initialize_fds();
  while (true) {
    evlist.resize(size);
    int nfd = epoll_wait(epfd, evlist.data(), evlist.size(), 0);
    if (nfd == -1) {
      if (errno == EINTR) { // (TLPI Section 21.5)
        log(LOG_INFO, "epoll_wait() is stopped by a signal");
        continue;
      }
      throw std::runtime_error("epoll_wait");
    }
    for (int i = 0; i < nfd; ++i) {
      // TODO: EPOLLIN, EPOLLOUT,  EPOLLHUP, EPOLLERR をcheckするべき?
      // EPOLLERRはthe other end of a FIFOのcloseやterminal hangupで起きる
      // EPOLLERR はwebservに関係あるか？
      process_event(evlist[i].data.fd, is_readable(evlist[i]),
                    is_writable(evlist[i]));
    }
    if (nfd == size && size * 2 < max_user_watches) {
      size *= 2;
    }
  }
}

void EpollMultiplexer::add_to_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.events = is_in_write_fds(fd) ? EPOLLIN | EPOLLOUT : EPOLLIN;
  ev.data.fd = fd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    if (errno != EEXIST) {
      logfd(LOG_ERROR, "failed to list fd in read monitor: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd already under monitor: ", fd);
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
      logfd(LOG_ERROR, "failed to list fd in read monitor: ", fd);
      return;
    }
  }
  read_fds.insert(fd);
}

void EpollMultiplexer::remove_from_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.events = 0;
  ev.data.fd = fd;

  if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev) == -1) {
    if (errno != ENOENT) {
      logfd(LOG_ERROR, "failed to delete fd: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd already erased: ", fd);
  }
  read_fds.erase(fd);
  write_fds.erase(fd);
}

void EpollMultiplexer::add_to_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN | EPOLLOUT;

  if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {

    if (errno != ENOENT) {
      logfd(LOG_ERROR, "failed to list fd in write monitor: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd not in read monitor. Adding it now: ", fd);
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
      logfd(LOG_ERROR, "failed to list fd in both monitor: ", fd);
      return;
    }
    read_fds.insert(fd);
  }
  write_fds.insert(fd);
}

void EpollMultiplexer::remove_from_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);

  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = EPOLLIN;

  if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {

    if (errno != ENOENT) {
      logfd(LOG_ERROR, "failed to delist fd in write monitor: ", fd);
      return;
    }
    logfd(LOG_WARNING, "fd not in read monitor. Adding it now: ", fd);
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
      logfd(LOG_ERROR, "failed to list fd in read monitor: ", fd);
      return;
    }
    read_fds.insert(fd);
  }
  write_fds.erase(fd);
}

bool EpollMultiplexer::is_readable(struct epoll_event &ev) const {
  return ev.events & EPOLLIN;
}

bool EpollMultiplexer::is_writable(struct epoll_event &ev) const {
  return ev.events & EPOLLOUT;
}

bool EpollMultiplexer::is_in_read_fds(int fd) const {
  return read_fds.find(fd) != read_fds.end();
}

bool EpollMultiplexer::is_in_write_fds(int fd) const {
  return write_fds.find(fd) != write_fds.end();
}

// EPOLLIN ではなく、buf超過のときにはerror handling必要かも？
// bool EpollMultiplexer::has_more_than_max_to_read(struct epoll_event &ev)
// const {
//   return ev.events & (EPOLLHUP | EPOLLERR);
// }

EpollMultiplexer::EpollMultiplexer() {}

EpollMultiplexer::EpollMultiplexer(const EpollMultiplexer &other)
    : Multiplexer(other) {}

EpollMultiplexer::~EpollMultiplexer() {}

EpollMultiplexer &EpollMultiplexer::operator=(const EpollMultiplexer &other) {
  (void)other;
  return *this;
}
