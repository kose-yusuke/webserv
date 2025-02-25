#pragma once

#include "Multiplexer.hpp"

/**
 * kqueue を用いたI/O多重化
 */
class KqueueMultiplexer : public Multiplexer {
public:
  static void run();

private:
  KqueueMultiplexer();
  KqueueMultiplexer(const KqueueMultiplexer &other);
  ~KqueueMultiplexer();
  KqueueMultiplexer &operator=(const KqueueMultiplexer &other);
};
