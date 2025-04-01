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
  void add_to_read_fds(int fd);
  void remove_from_read_fds(int fd);
  void add_to_write_fds(int fd);
  void remove_from_write_fds(int fd);

private:
  typedef std::set<int> FdBackup;
  typedef std::set<int>::iterator FdBackupIt;
  typedef std::set<int>::const_iterator ConstFdBackupIt;

  int epfd;
  std::set<int> read_fds;
  std::set<int> write_fds;

  bool is_readable(struct epoll_event &ev) const;
  bool is_writable(struct epoll_event &ev) const;
  // bool has_more_than_max_to_read(struct epoll_event &ev) const;

  bool is_in_read_fds(int fd) const;
  bool is_in_write_fds(int fd) const;

  EpollMultiplexer();
  EpollMultiplexer(const EpollMultiplexer &other);
  ~EpollMultiplexer();

  EpollMultiplexer &operator=(const EpollMultiplexer &other);
};

// TODO: dup()して、fdの複製が生じたときの管理
// note: edge-triggered にするにはev.events = EPOLLIN | EPOLLET;する
/*
TODO: max_user_watches の反映
c5r1s4% cat /proc/sys/fs/epoll/max_user_watches
3599293
TODO: handling EPOLLRDHUP, EPOLLHUP, EPOLLERR は大丈夫か？
*/
// add, remove に失敗したfdはどこでcloseするべきか？
