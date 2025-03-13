#include "EpollMultiplexer.hpp"
#include <cstdlib>
#include <iostream>

Multiplexer &EpollMultiplexer::get_instance() {
  if (!Multiplexer::instance) {
    Multiplexer::instance = new EpollMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance;
}

void EpollMultiplexer::run() {
  std::cout << "EpollMultiplexer::run() called\n";
}

void EpollMultiplexer::add_to_read_fds(int fd) { (void)fd; }
void EpollMultiplexer::remove_from_read_fds(int fd) { (void)fd; }
void EpollMultiplexer::add_to_write_fds(int fd) { (void)fd; }
void EpollMultiplexer::remove_from_write_fds(int fd) { (void)fd; }

bool EpollMultiplexer::is_readable(int fd) {
  (void)fd;
  return true;
}
bool EpollMultiplexer::is_writable(int fd) {
  (void)fd;
  return true;
}

EpollMultiplexer::EpollMultiplexer() {}

EpollMultiplexer::EpollMultiplexer(const EpollMultiplexer &other)
    : Multiplexer(other) {}

EpollMultiplexer::~EpollMultiplexer() {}

EpollMultiplexer &EpollMultiplexer::operator=(const EpollMultiplexer &other) {
  (void)other;
  return *this;
}
