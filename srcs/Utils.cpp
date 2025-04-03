/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:21 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/04/02 13:01:43 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"
#include <limits>

int print_error_message(const std::string &message) {
  std::cerr << "Error: " << message << std::endl;
  std::exit(1);
  return (1);
}

bool ends_with(const std::string &str, const std::string &suffix) {
  if (str.length() < suffix.length())
    return false;
  return (str.compare(str.length() - suffix.length(), suffix.length(),
                      suffix) == 0);
}

bool file_exists(const std::string &path) {
  return (access(path.c_str(), F_OK) == 0);
}

bool has_index_file(const std::string &dir_path) {
  std::string index_file = dir_path + "/index.html";
  return file_exists(index_file);
}

bool is_directory(const std::string &path) {
  struct stat buffer;
  if (stat(path.c_str(), &buffer) != 0) {
    return false;
  }
  return S_ISDIR(buffer.st_mode);
}

std::string read_file(const std::string &file_path) {
  std::ifstream file(file_path.c_str(), std::ios::in);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + file_path);
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// 拡張子に基づいてMIMEタイプを返す関数
// std::string get_mime_type(const std::string& file_path) {
//     // 拡張子とMIMEタイプのマッピング
//     static const std::map<std::string, std::string> mime_types = {
//         {".html", "text/html"},
//         {".css", "text/css"},
//         {".js", "application/javascript"},
//         {".png", "image/png"},
//         {".jpg", "image/jpeg"},
//         {".jpeg", "image/jpeg"},
//         {".gif", "image/gif"},
//         {".svg", "image/svg+xml"},
//         {".json", "application/json"},
//         {".txt", "text/plain"}
//     };

//     // ファイルパスから拡張子を抽出
//     size_t dot_pos = file_path.find_last_of('.');
//     if (dot_pos != std::string::npos) {
//         std::string extension = file_path.substr(dot_pos);
//         if (mime_types.find(extension) != mime_types.end()) {
//             return mime_types.at(extension);
//         }
//     }

//     // デフォルトのMIMEタイプ
//     return "application/octet-stream";
// }

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

LogLevel current_log_level = static_cast<LogLevel>(LOG_LEVEL);

std::ofstream debug_log("debug.log");

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
      debug_log << "[DEBUG] " << message << std::endl;
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

std::string trim_left(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string trim_right(const std::string &s) {
  size_t end = s.find_last_not_of(" \t\r\n");
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string &s) {
  std::string trimmed = trim_right(s);
  return trim_left(trimmed);
}

int str_to_int(const std::string &s) {
  char *endptr;
  errno = 0;

  long val = std::strtol(s.c_str(), &endptr, 10);
  if (errno != 0 || endptr == s.c_str()) {
    throw std::runtime_error("Invalid string to convert to number: " + s);
  }
  if (*endptr != '\0') {
    throw std::runtime_error("Invalid suffix in number: " + s);
  }
  if (val < std::numeric_limits<int>::min() ||
      val > std::numeric_limits<int>::max()) {
    throw std::runtime_error("Value out of range for int: " + s);
  }
  return val;
}

size_t str_to_size(const std::string &s) {
  char *endptr;
  errno = 0;

  unsigned long val = std::strtoul(s.c_str(), &endptr, 10);
  if (errno != 0 || endptr == s.c_str()) {
    throw std::runtime_error("Invalid string to convert to number: " + s);
  }
  if (*endptr == '\0') {
    return val;
  }
  if ((endptr[0] == 'M' || endptr[0] == 'm') && endptr[1] == '\0') {
    if (val > std::numeric_limits<std::size_t>::max() / 1048576) {
      throw std::runtime_error("Overflow during conversion: " + s);
    }
    return val * 1048576;
  }
  throw std::runtime_error("Invalid suffix in number: " + s);
}

size_t parse_hex(const std::string &s) {
  char *endptr;
  errno = 0;

  unsigned long val = std::strtoul(s.c_str(), &endptr, 16);
  if (errno != 0 || endptr == s.c_str() || *endptr != '\0') {
    throw std::runtime_error("Invalid chunk-size: " + s);
  }
  return val;
}
