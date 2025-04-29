#pragma once

#include <string>

class SocketBuilder {
public:
  static int create_socket(const std::string &ip, const std::string &port);
};
