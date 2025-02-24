#pragma once

#include "Multiplexer.hpp"
#include <sys/select.h>

/**
 * select を用いたI/O多重化
 */
class SelectMultiplexer : public Multiplexer {
public:
  static void run();

private:
  static void addFd(fd_set &fdSet, int &maxFd, int fd);
  static void removeFd(fd_set &fdSet, int &maxFd, int fd);
  static void addAllServerFdsToFdSet(fd_set &fdSet, int &maxFd);

  static void acceptClient(fd_set &fdSet, int &maxFd, int serverFd);
  static void handleClient(fd_set &fdSet, int &maxFd, int clientFd);

  SelectMultiplexer();
  SelectMultiplexer(const SelectMultiplexer &other);
  ~SelectMultiplexer();
  SelectMultiplexer &operator=(const SelectMultiplexer &other);
};
