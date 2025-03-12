#pragma once

#include "Multiplexer.hpp"

/**
 * epoll を用いたI/Oの多重化
 */
class EpollMultiplexer : public Multiplexer {
public:
  EpollMultiplexer();
  ~EpollMultiplexer();

  void run();

private:
  EpollMultiplexer(const EpollMultiplexer &other);
  EpollMultiplexer &operator=(const EpollMultiplexer &other);
  ;
};
