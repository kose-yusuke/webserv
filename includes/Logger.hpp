#pragma once

#include <fstream>
#include <iostream>
#include <string>

enum LogLevel { LOG_FUNC, LOG_INFO, LOG_DEBUG, LOG_WARNING, LOG_ERROR };

extern LogLevel current_log_level;

void log(LogLevel level, const std::string &message);
void logfd(LogLevel level, const std::string &prefix, int fd);

#define LOG_DEBUG_FUNC() log(LOG_FUNC, std::string(__func__) + "() called")
#define LOG_DEBUG_FUNC_FD(fd)                                                  \
  logfd(LOG_FUNC, std::string(__func__) + "() called on fd: ", fd)

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define RESET "\033[0m"
