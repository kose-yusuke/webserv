/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:21 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/20 15:56:52 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"

int print_error_message(const std::string &message) {
  std::cerr << "Error: " << message << std::endl;
  exit(1);
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

LogLevel current_log_level = LOG_INFO;
std::ofstream debug_log("debug.log");

void log(LogLevel level, const std::string &message) {
  if (level >= current_log_level) {
    switch (level) {
    case LOG_INFO:
      std::cout << "[INFO] " << message << std::endl;
      break;
    case LOG_DEBUG:
      debug_log << "[DEBUG] " << message << std::endl;
      break;
    case LOG_ERROR:
      std::cerr << "[ERROR] " << message << std::endl;
      break;
    }
  }
}
