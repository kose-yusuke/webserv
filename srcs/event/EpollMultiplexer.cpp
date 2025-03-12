#include "EpollMultiplexer.hpp"

void EpollMultiplexer::run() {}

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
