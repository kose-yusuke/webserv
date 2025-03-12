#pragma once

#include "Multiplexer.hpp"
#include <poll.h>
#include <vector>

/**
 * poll を用いたI/O多重化
 */
class PollMultiplexer : public Multiplexer {
public:
  PollMultiplexer();
  ~PollMultiplexer();

  void run();

private:
  // std::vector<struct pollfd> &pfds;

  void addPfd(std::vector<struct pollfd> &pfds, int fd);
  void removePfd(std::vector<struct pollfd> &pfds, int fd);
  bool isInPfds(const std::vector<struct pollfd> &pfds, int fd);
  void addAllServerFdsToPfds(std::vector<struct pollfd> &pfds);

  void acceptClient(std::vector<struct pollfd> &pfds, int serverFd);
  void handleClient(std::vector<struct pollfd> &pfds, int clientFd);

  PollMultiplexer(const PollMultiplexer &other);
  PollMultiplexer &operator=(const PollMultiplexer &other);
};
