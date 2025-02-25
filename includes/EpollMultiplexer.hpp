#pragma once

#include "Multiplexer.hpp"

/**
 * epoll を用いたI/Oの多重化
 */
class EpollMultiplexer : public Multiplexer {
public:
  static void run();

private:
  EpollMultiplexer();
  EpollMultiplexer(const EpollMultiplexer &other);
  ~EpollMultiplexer();
  EpollMultiplexer &operator=(const EpollMultiplexer &other);
  ;
};
