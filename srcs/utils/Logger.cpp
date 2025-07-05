#include "Logger.hpp"
#include <cerrno>
#include <cstring>
#include <sstream>

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

LogLevel current_log_level = static_cast<LogLevel>(LOG_LEVEL);

void log(LogLevel level, const std::string &message) {
  if (level >= current_log_level) {
    switch (level) {
    case LOG_FUNC:
      std::cout << CYAN << "[FUNC] " << RESET << message << std::endl;
      break;
    case LOG_INFO:
      std::cout << GREEN << "[INFO] " << RESET << message << std::endl;
      break;
    case LOG_DEBUG:
      break;
    case LOG_WARNING:
      std::cerr << YELLOW << "[WARNING] " << RESET << message << std::endl;
      break;
    case LOG_ERROR:
      std::cerr << RED << "[ERROR] " << RESET << message;
      if (errno != 0) {
        int err = errno;
        errno = 0;
        std::cerr << " (errno: " << err << ", reason: " << strerror(err) << ")";
      }
      std::cerr << std::endl;
      break;
    default:
      std::cerr << "[UNKNOWN] " << message << std::endl;
    }
  }
}

void logfd(LogLevel level, const std::string &message, int fd) {
  std::ostringstream oss;
  oss << message << fd;
  log(level, oss.str());
}
