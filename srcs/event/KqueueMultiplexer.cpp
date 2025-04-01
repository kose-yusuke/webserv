#include "KqueueMultiplexer.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <iostream>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>

Multiplexer &KqueueMultiplexer::get_instance() {
  if (!Multiplexer::instance) {
    log(LOG_INFO, "KqueueMultiplexer::get_instance()");
    Multiplexer::instance = new KqueueMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance;
}

void KqueueMultiplexer::run() {
  LOG_DEBUG_FUNC();
  static const int max_kqueue_events = 16384;
  int kq = kqueue();
  int size = 16;
  initialize_fds();

  while (true) {
    if (size >= max_kqueue_events) {
      throw std::runtime_error("kqueue event list exceeds limit");
    }
    event_list.resize(size);
    int nfd = kevent(kq, change_list.data(), change_list.size(),
                     event_list.data(), event_list.size(), 0);
    if (nfd == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("kqueue() failed");
    }

    logfd(LOG_DEBUG, "kqueue() returned: ", nfd);

    change_list.clear();
    for (int i = 0; i < nfd; ++i) {
      process_event(event_list[i].ident, is_readable(event_list[i]),
                    is_writable(event_list[i]));
    }
    if (nfd == size) {
      size *= 2;
    }
  }
}

void KqueueMultiplexer::add_to_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  struct kevent ev;
  EV_SET(&ev, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
  change_list.push_back(ev);
}

void KqueueMultiplexer::remove_from_read_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  struct kevent ev;
  EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
  change_list.push_back(ev);
}

void KqueueMultiplexer::add_to_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  struct kevent ev;
  EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
  change_list.push_back(ev);
}

void KqueueMultiplexer::remove_from_write_fds(int fd) {
  LOG_DEBUG_FUNC_FD(fd);
  struct kevent ev;
  EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
  change_list.push_back(ev);
}

bool KqueueMultiplexer::is_readable(struct kevent &ev) const {
  return (ev.filter == EVFILT_READ);
}

bool KqueueMultiplexer::is_writable(struct kevent &ev) const {
  return (ev.filter == EVFILT_WRITE);
}

KqueueMultiplexer::KqueueMultiplexer() {}

KqueueMultiplexer::KqueueMultiplexer(const KqueueMultiplexer &other)
    : Multiplexer(other) {
  (void)other;
}

KqueueMultiplexer::~KqueueMultiplexer() {}

KqueueMultiplexer &
KqueueMultiplexer::operator=(const KqueueMultiplexer &other) {
  (void)other;
  return *this;
}
