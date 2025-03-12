#pragma once

#include "Multiplexer.hpp"

/**
 * kqueue を用いたI/O多重化
 */
class KqueueMultiplexer : public Multiplexer {
public:
  KqueueMultiplexer();
  ~KqueueMultiplexer();

  void run();

private:
  KqueueMultiplexer(const KqueueMultiplexer &other);
  KqueueMultiplexer &operator=(const KqueueMultiplexer &other);
};
