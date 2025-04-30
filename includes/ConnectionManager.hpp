#pragma once

class Client;

class ConnectionManager {
public:
  static int accept_new_connection(int server_fd);
};
