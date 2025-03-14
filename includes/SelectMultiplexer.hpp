#pragma once

#include "Multiplexer.hpp"
#include <sys/select.h>

/**
 * select を用いたI/O多重化
 */
class SelectMultiplexer : public Multiplexer {
public:
  static Multiplexer &get_instance();

  void run();

protected:
  void add_to_read_fds(int fd);
  void remove_from_read_fds(int fd);
  void add_to_write_fds(int fd);
  void remove_from_write_fds(int fd);

private:
  fd_set read_fds;  // 常時監視
  fd_set write_fds; // 必要時監視

  fd_set active_read_fds;
  fd_set active_write_fds;

  int max_fd;

  bool is_readable(int fd);
  bool is_writable(int fd);

  SelectMultiplexer();
  SelectMultiplexer(const SelectMultiplexer &other);
  ~SelectMultiplexer();

  SelectMultiplexer &operator=(const SelectMultiplexer &other);
};
