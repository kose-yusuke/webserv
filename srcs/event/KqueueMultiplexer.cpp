#include "KqueueMultiplexer.hpp"
#include "PollMultiplexer.hpp"
#include <iostream>

void KqueueMultiplexer::run() {
  std::cout << "KqueueMultiplexer is not ready !!\nCalling poll ...\n";
  PollMultiplexer::run();
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
