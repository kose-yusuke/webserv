#pragma once

#include "Multiplexer.hpp"
#include <sys/select.h>

/**
 * select を用いたI/O多重化
 */
class SelectMultiplexer : public Multiplexer {
public:
  SelectMultiplexer();
  ~SelectMultiplexer();

  void run();

private:
  fd_set read_fds;        // 継続的に監視するfd
  fd_set active_read_fds; // select() の結果を保存するfd

  fd_set write_fds;        // 継続的に監視するfd
  fd_set active_write_fds; // select() の結果を保存するfd

  int max_fd;

  void initialize_fds();

  void add_to_read_fds(int fd);
  void remove_from_read_fds(int fd);
  bool is_readable(int fd);

  void add_to_write_fds(int fd);
  void remove_from_write_fds(int fd);
  bool is_writable(int fd);

  void accept_client(int server_fd);
  void handle_client(int client_fd);
  void remove_client(int client_fd);

  SelectMultiplexer(const SelectMultiplexer &other);
  SelectMultiplexer &operator=(const SelectMultiplexer &other);
};
