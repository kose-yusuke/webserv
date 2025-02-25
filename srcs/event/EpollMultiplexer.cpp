#include "EpollMultiplexer.hpp"
#include "PollMultiplexer.hpp"
#include <iostream>

#if defined(__linux__)
#include <sys/epoll.h>
#endif

void EpollMultiplexer::run() {
  std::cout << "EpollMultiplexer is not ready !!\nCalling poll ...\n";
  PollMultiplexer::run();
#if defined(__linux__)
  int epfd, ready, fd, s, j, num0penFds;
  struct epoll_event ev;
  struct epoll_event evlist[MAX_EVENTS];
  char buf[MAX_BUF];

  epfd = epoll_create(serverFdMap_.size() + 5);

  for (std::map<int, Server *>::iterator it = serverFdMap_.begin();
       it != serverFdMap_.end(); ++i) {
    ev.data.fd = it->first;
    ev.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
      perror("epoll_ctl");
  }

  numOpenFds = argc - 1;
  while (numOpenFds > 0) {
    printf("About to epoll_wait()\n");
    ready = epoll_wait(epfd, evlist, MAX_EVENTS, -1);
    if (ready == -1) {
      if (errno == EINTR)
        continue;
      else
        perror("epoll_wait");
    }

    printf("Ready: %d\n", ready);
    for (j = 0; j < ready; j++) {
      printf(" fd=%d; events: %s%s%s\n", evlist[j].data.fd,
             (evlist[j].events & EPOLLIN) ? "EPOLLIN " : "",
             (evlist[j].events & EPOLLHUP) ? "EPOLLHUP " : "",
             (evlist[j].events & EPOLLERR) ? "EPOLLERR " : "");
      if (evlist[j].events & EPOLLIN) {
        s = read(evlist[j].data.fd, buf, MAX_BUF);
        if (s == -1)
          perror("read");
        printf(" read %d bytes: %.*s\n", s, s, buf);

      } else if (evlist[j].events & (EPOLLHUP | EPOLLERR)) {

        printf(" closing fd %d\n", evlist[j].data.fd);
        if (close(evlist[j].data.fd) == -1)
          perror("close");
        numOpenFds--;
      }
    }
    63.4.4
  }
}
#endif
}

EpollMultiplexer::EpollMultiplexer() {}

EpollMultiplexer::EpollMultiplexer(const EpollMultiplexer &other)
    : Multiplexer(other) {
  (void)other;
}

EpollMultiplexer::~EpollMultiplexer() {}

EpollMultiplexer &EpollMultiplexer::operator=(const EpollMultiplexer &other) {
  (void)other;
  return *this;
}
