#include "EpollMultiplexer.hpp"
#include "PollMultiplexer.hpp"
#include <iostream>

void EpollMultiplexer::run() {
  std::cout << "EpollMultiplexer is not ready !!\nCalling poll ...\n";
  PollMultiplexer::run();
}

EpollMultiplexer::EpollMultiplexer() {}

EpollMultiplexer::EpollMultiplexer(const EpollMultiplexer &other) {
  (void)other;
}

EpollMultiplexer::~EpollMultiplexer() {}

EpollMultiplexer &EpollMultiplexer::operator=(const EpollMultiplexer &other) {
  (void)other;
  return *this;
}
