#pragma once

#include "Multiplexer.hpp"
#include <poll.h>
#include <vector>

/**
 * poll を用いた I/O 多重化
 */
class PollMultiplexer : public Multiplexer {
public:
  static Multiplexer &get_instance();

  void run();

protected:
  void monitor_read(int fd);
  void monitor_write(int fd);
  void unmonitor_write(int fd);
  void unmonitor(int fd);

private:
  typedef std::vector<struct pollfd> PollFdVec;
  typedef std::vector<struct pollfd>::iterator PollFdIt;
  typedef std::vector<struct pollfd>::const_iterator ConstPollFdIt;

  PollFdVec pfds;

  bool is_readable(struct pollfd fd) const;
  bool is_writable(struct pollfd fd) const;

  PollFdIt find_pollfd(int fd);

  PollMultiplexer();
  PollMultiplexer(const PollMultiplexer &other);
  ~PollMultiplexer();

  PollMultiplexer &operator=(const PollMultiplexer &other);
};
