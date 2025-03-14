#pragma once

#include "Multiplexer.hpp"

/**
 * kqueue を用いたI/O多重化
 */
class KqueueMultiplexer : public Multiplexer {
public:
  static Multiplexer &get_instance();

  void run();

protected:
  void add_to_read_fds(int fd);
  void remove_from_read_fds(int fd);
  void add_to_write_fds(int fd);
  void remove_from_write_fds(int fd);

private:
  bool is_readable(int fd);
  bool is_writable(int fd);

  KqueueMultiplexer();
  KqueueMultiplexer(const KqueueMultiplexer &other);
  ~KqueueMultiplexer();

  KqueueMultiplexer &operator=(const KqueueMultiplexer &other);
};
