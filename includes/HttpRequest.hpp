/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:38 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/12 16:38:51 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

enum MethodType { GET, POST, DELETE };

class HttpRequest {
public:
  // メンバ変数（仮）
  std::string method;
  std::string path;
  std::string version;
  std::string body;
  std::map<std::string, std::string> headers;

  MethodType methodType_;

  // TODO: 未作成の関数群
  HttpRequest();
  size_t get_content_length();
  bool is_header_received();
  void parse_header(const std::string &request);
  void parse_body(const std::string &request);
  void clear();
  // ここまで未完成

  void handleHttpRequest(int clientFd, const char *buffer, int nbytes);

  // リクエストの解析
  bool parse_http_request(const std::string &request, std::string &method,
                          std::string &path, std::string &version);

  // GET, POST, DELETE の処理
  void handle_get_request(int client_socket, std::string path);
  void handle_post_request(int client_socket, const std::string &request);
  void handle_delete_request();
  std::string read_file(const std::string &file_path);
};
