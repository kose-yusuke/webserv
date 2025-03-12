#include "SelectMultiplexer.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

void SelectMultiplexer::run() {
  std::cout << "SelectMultiplexer::run() called\n";
  initialize_fds();
  while (true) {
    active_read_fds = read_fds;
    active_write_fds = write_fds;
    int ret = select(max_fd + 1, &active_read_fds, &active_write_fds, 0, 0);
    std::cout << "Select returned: " << ret << "\n";
    if (ret < 0 && errno != EINTR) {
      std::cerr << "Error: select() failed: " << strerror(errno) << "\n";
      continue;
    }
    for (int event_fd = 0; event_fd <= max_fd; ++event_fd) {
      if (!is_readable(event_fd) && !is_writable(event_fd)) {
        continue;
      }
      if (is_in_client_map(event_fd)) {
        handle_client(event_fd);
      } else if (is_in_server_map(event_fd)) {
        accept_client(event_fd);
      }
    }
  }
}

// XXX: この部分はmultiplexerに集約できるかも
void SelectMultiplexer::initialize_fds() {
  std::cout << "initialize_fds() called\n";
  if (server_map.empty()) {
    throw std::runtime_error("Error: No servers available.");
  }
  for (ServerIt it = server_map.begin(); it != server_map.end(); ++it) {
    add_to_read_fds(it->first);
  }
}

void SelectMultiplexer::add_to_read_fds(int fd) {
  std::cout << "add_to_read_fds() called on " << fd << "\n";
  FD_SET(fd, &read_fds);
  max_fd = std::max(fd, max_fd);
}

void SelectMultiplexer::remove_from_read_fds(int fd) {
  std::cout << "remove_from_read_fds() called on " << fd << "\n";
  FD_CLR(fd, &read_fds);
  if (fd != max_fd) {
    return;
  }
  for (int tmp_fd = fd - 1; tmp_fd >= 0; --tmp_fd) {
    if (FD_ISSET(tmp_fd, &read_fds)) {
      max_fd = tmp_fd;
      break;
    }
  }
}

bool SelectMultiplexer::is_readable(int fd) {
  return FD_ISSET(fd, &active_read_fds);
}

void SelectMultiplexer::add_to_write_fds(int fd) {
  std::cout << "add_to_write_fds() called on " << fd << "\n";
  FD_SET(fd, &write_fds);
}

void SelectMultiplexer::remove_from_write_fds(int fd) {
  std::cout << "remove_from_write_fds() called on " << fd << "\n";
  FD_CLR(fd, &write_fds);
}

bool SelectMultiplexer::is_writable(int fd) {
  return FD_ISSET(fd, &active_write_fds);
}

void SelectMultiplexer::accept_client(int server_fd) {
  std::cout << "accept_client() called on server fd " << server_fd << "\n";
  if (!is_readable(server_fd)) {
    std::cout << "false\n";
    return;
  }
  struct sockaddr_storage client_addr;
  socklen_t addrlen = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
  if (client_fd == -1) {
    std::cerr << "accept failed\n";
    return;
  }
  try {
    add_to_client_map(client_fd, new Client(client_fd, server_fd));
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    close(client_fd);
    return;
  }
  add_to_read_fds(client_fd);
  std::cout << "New connection on client fd: " << client_fd << "\n";
}

void SelectMultiplexer::handle_client(int client_fd) {
  std::cout << "handle_client() called on client fd " << client_fd << "\n";
  Client *client = get_client_from_map(client_fd);
  IOStatus status;

  // XXX: read or write どちらを優先するべきか確認
  if (is_readable(client_fd)) {
    std::cout << "reable == true\n";
    status = client->on_read();
    if (status == IO_SUCCESS) {
      std::cout << "success\n";
      add_to_write_fds(client_fd);
    } else if (status == IO_FAILED) {
      std::cout << "failed\n";
      remove_client(client_fd);
    }
  } else if (is_writable(client_fd)) {
    std::cout << "writable == true\n";
    status = client->on_write();
    if (status == IO_SUCCESS) {
      std::cout << "success\n";
      remove_from_write_fds(client_fd);
    } else if (status == IO_FAILED) {
      std::cout << "failed\n";
      remove_client(client_fd);
    }
  }
}

void SelectMultiplexer::remove_client(int client_fd) {
  std::cout << "remove_client() called on client fd" << client_fd << "\n";
  remove_from_write_fds(client_fd);
  remove_from_read_fds(client_fd);
  remove_from_client_map(client_fd); // close, delete もこの中
}

SelectMultiplexer::SelectMultiplexer() : Multiplexer(), max_fd(0) {
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
}

SelectMultiplexer::SelectMultiplexer(const SelectMultiplexer &other)
    : Multiplexer(other) {}

SelectMultiplexer::~SelectMultiplexer() {}

SelectMultiplexer &
SelectMultiplexer::operator=(const SelectMultiplexer &other) {
  (void)other;
  return *this;
}
