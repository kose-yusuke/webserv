#include "KqueueMultiplexer.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <iostream>

Multiplexer &KqueueMultiplexer::get_instance() {
  if (!Multiplexer::instance) {
    Multiplexer::instance = new KqueueMultiplexer();
    std::atexit(Multiplexer::delete_instance);
  }
  return *Multiplexer::instance;
}

void KqueueMultiplexer::run() {
  log(LOG_INFO, "KqueueMultiplexer::run() called");
}

void KqueueMultiplexer::add_to_read_fds(int fd) { (void)fd; }
void KqueueMultiplexer::remove_from_read_fds(int fd) { (void)fd; }
void KqueueMultiplexer::add_to_write_fds(int fd) { (void)fd; }
void KqueueMultiplexer::remove_from_write_fds(int fd) { (void)fd; }

bool KqueueMultiplexer::is_readable(int fd) {
  (void)fd;
  return true;
}

bool KqueueMultiplexer::is_writable(int fd) {
  (void)fd;
  return true;
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
