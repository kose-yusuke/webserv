/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:21 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/05/17 19:08:59 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"
#include <limits>

int print_error_message(const std::string &message) {
  std::cerr << "Error: " << message << std::endl;
  std::exit(1);
  return (1);
}

void print_best_match_config(ConfigMap best_match_config){
  LOG_DEBUG_FUNC();
  std::cout << "=== best_match_config ===" << std::endl;
  for (ConstConfigIt it = best_match_config.begin();
       it != best_match_config.end(); ++it) {
    std::cout << "Key: " << it->first << std::endl;
    std::cout << "Values:";
    for (std::vector<std::string>::const_iterator vit = it->second.begin();
         vit != it->second.end(); ++vit) {
      std::cout << " " << *vit;
    }
    std::cout << std::endl;
  }
  std::cout << "==================================" << std::endl;
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

bool has_index_file(const std::string &dir_path, std::string index_file_name) {
  std::string index_file = dir_path + index_file_name;
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

std::string make_unique_filename() {
  std::stringstream ss;
  ss << getpid() << "_" << std::rand();
  return ss.str();
}

bool is_all_digits(const std::string &str) {
  for (size_t i = 0; i < str.size(); ++i) {
    if (!std::isdigit(str[i]))
      return false;
  }
  return true;
}

bool regex_match_posix(const std::string &text, const std::string &pattern, bool ignore_case) {
  regex_t regex;
  int cflags = REG_EXTENDED;
  if (ignore_case)
  cflags |= REG_ICASE;

  if (regcomp(&regex, pattern.c_str(), cflags) != 0)
  return false;

  int result = regexec(&regex, text.c_str(), 0, NULL, 0);
  regfree(&regex);
  return result == 0;
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

std::vector<std::string> split_csv(const std::string &value) {
  std::vector<std::string> result;
  std::stringstream ss(value);
  std::string item;
  while (std::getline(ss, item, ',')) {
    result.push_back(trim(item));
  }
  return result;
}

std::string to_lower(const std::string &s) {
  std::string result = s;
  for (size_t i = 0; i < result.length(); ++i) {
    result[i] =
        static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
  }
  return result;
}
