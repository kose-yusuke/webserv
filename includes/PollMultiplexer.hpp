#pragma once

#include "Multiplexer.hpp"
#include <poll.h>
#include <vector>

/**
 * poll を用いたI/O多重化
 */
class PollMultiplexer : public Multiplexer {
public:
  static void run();

private:
  static void addPfd(std::vector<struct pollfd> &pfds, int fd);
  static void removePfd(std::vector<struct pollfd> &pfds, int fd);
  static bool isInPfds(const std::vector<struct pollfd> &pfds, int fd);
  static void addAllServerFdsToPfds(std::vector<struct pollfd> &pfds);

  static void acceptClient(std::vector<struct pollfd> &pfds, int serverFd);
  static void handleClient(std::vector<struct pollfd> &pfds, int clientFd);

  PollMultiplexer();
  PollMultiplexer(const PollMultiplexer &other);
  ~PollMultiplexer();
  PollMultiplexer &operator=(const PollMultiplexer &other);
};
