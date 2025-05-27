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
  void monitor_read(int fd);
  void monitor_write(int fd);
  void unmonitor_write(int fd);
  void unmonitor(int fd);
  void monitor_pipe_read(int fd);
  void monitor_pipe_write(int fd);

private:
  typedef std::vector<struct kevent> KeventVec;
  typedef std::vector<struct kevent>::iterator KeventIt;
  typedef std::vector<struct kevent>::const_iterator ConstKeventIt;

  KeventVec change_list;
  KeventVec event_list;

  bool is_readable(struct kevent &ev) const;
  bool is_writable(struct kevent &ev) const;

  KqueueMultiplexer();
  KqueueMultiplexer(const KqueueMultiplexer &other);
  ~KqueueMultiplexer();

  KqueueMultiplexer &operator=(const KqueueMultiplexer &other);
};
