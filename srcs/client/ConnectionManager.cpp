#include "ConnectionManager.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include <fcntl.h>

int ConnectionManager::accept_new_connection(int server_fd) {
  LOG_DEBUG_FUNC_FD(server_fd);
  struct sockaddr_storage client_addr;
  socklen_t addrlen = sizeof(client_addr);
  int new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
  if (new_fd == -1) {
    logfd(LOG_ERROR, "Failed to accept new connection on socket: ", server_fd);
    return -1;
  }
  if (fcntl(new_fd, F_SETFL, fcntl(new_fd, F_GETFL) | O_NONBLOCK) == -1) {
    logfd(LOG_ERROR, "Failed to set O_NONBLOCK on socket: ", new_fd);
    close(new_fd);
    return -1;
  }
  int opt = 1;
  if (setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    logfd(LOG_ERROR, "Failed to set SO_REUSEADDR on socket: ", new_fd);
    close(new_fd);
    return -1;
  }
  return new_fd;
}
