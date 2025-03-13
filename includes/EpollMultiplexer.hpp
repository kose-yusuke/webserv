#pragma once

#include "Multiplexer.hpp"

/**
 * epoll を用いたI/Oの多重化
 */
class EpollMultiplexer : public Multiplexer {
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

  EpollMultiplexer();
  EpollMultiplexer(const EpollMultiplexer &other);
  ~EpollMultiplexer();

  EpollMultiplexer &operator=(const EpollMultiplexer &other);
  ;
};
