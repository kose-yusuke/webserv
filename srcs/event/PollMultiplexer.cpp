#include "PollMultiplexer.hpp"
#include "Server.hpp"
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/socket.h>

Multiplexer &PollMultiplexer::get_instance() {
  if (!Multiplexer::instance) {
    Multiplexer::instance = new PollMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance;
}

void PollMultiplexer::run() {
  std::cout << "PollMultiplexer::run() called\n";
  pfds.reserve(get_num_servers());
  initialize_fds();
  if (pfds.empty()) {
    throw std::runtime_error("pollfd empty");
  }
  while (true) {
    int ready;
    do {
      ready = poll(pfds.data(), pfds.size(), 0);
    } while (ready == -1 && errno == EINTR);
    if (ready == -1) {
      std::ostringstream oss;
      oss << "Error: poll failed with errno " << strerror(errno);
      throw std::runtime_error(oss.str());
    }
    PollFdVec tmp = pfds;
    for (size_t i = 0; i < tmp.size(); ++i) {
      process_event(tmp[i].fd, is_readable(tmp[i]), is_writable(tmp[i]));
    }
  }
}

void PollMultiplexer::add_to_read_fds(int fd) {
  std::cout << "add_to_read_fds() called on " << fd << "\n";
  PollFdIt it = find_pollfd(fd);
  if (it != pfds.end()) {
    std::cerr << "Warning: fd " << fd << " is already registered\n";
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
  std::cout << "remove_from_read_fds() called on " << fd << "\n";
  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    std::cerr << "Warning: fd " << fd << " is already erased\n";
    return;
  }
  pfds.erase(it);
}

void PollMultiplexer::add_to_write_fds(int fd) {
  std::cout << "add_to_write_fds() called on " << fd << "\n";
  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    std::cerr << "Warning: fd " << fd
              << " is not in read monitor. Adding it now.\n";
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLOUT;
    pfds.push_back(pfd);
    return;
  }
  if (it->events & POLLOUT) {
    std::cerr << "Warning: fd " << fd
              << " is already registered for write monitor\n";
    return;
  }
  it->events |= POLLOUT;
}

void PollMultiplexer::remove_from_write_fds(int fd) {
  std::cout << "remove_from_write_fds() called on " << fd << "\n";
  PollFdIt it = find_pollfd(fd);
  if (it == pfds.end()) {
    std::cerr << "Warning: fd " << fd << " doesn't exist in pfds vector.\n";
    return;
  }
  if (!(it->events & POLLOUT)) {
    std::cerr << "Warning: fd " << fd << " is already not in write monitor.\n";
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
