#pragma once

#include "Multiplexer.hpp"
#include <set>
#include <vector>
#ifdef __linux__
#include <sys/epoll.h>
#endif

/**
 * epoll を用いたI/Oの多重化
 */
class EpollMultiplexer : public Multiplexer {
public:
  static Multiplexer &get_instance();

  void run();

protected:
  void monitor_read(int fd);
  void monitor_write(int fd);
  void unmonitor_write(int fd);
  void unmonitor(int fd);

private:
  typedef std::set<int> FdBackup;
  typedef std::set<int>::iterator FdBackupIt;
  typedef std::set<int>::const_iterator ConstFdBackupIt;

  int epfd_;
  std::set<int> read_fds_;
  std::set<int> write_fds_;

  bool is_readable(struct epoll_event &ev) const;
  bool is_writable(struct epoll_event &ev) const;

  bool is_in_read_fds(int fd) const;
  bool is_in_write_fds(int fd) const;

  EpollMultiplexer();
  EpollMultiplexer(const EpollMultiplexer &other);
  ~EpollMultiplexer();

  EpollMultiplexer &operator=(const EpollMultiplexer &other);
};
