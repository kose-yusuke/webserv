/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:38 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/04/16 17:49:01 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "types.hpp"
#include <algorithm>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <regex.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h> 

class HttpResponse;

enum ResourceType { File, Directory, NotFound };
enum RedirStatus { REDIR_NONE, REDIR_SUCCESS, REDIR_FAILED };

class HttpRequest {
public:
  // メンバ変数（仮）
  std::string method;
  std::string path;
  std::string version;
  // std::string body;
  std::vector<char> body_data;
  std::map<std::string, std::string> headers;

  bool is_autoindex_enabled;
  std::string index_file_name;
  std::vector<std::string> cgi_extensions;
  std::vector<std::string> allow_methods;
  std::map<int, std::string> error_page_map;

  ConfigMap server_config;
  LocationMap location_configs;
  ConfigMap best_match_config;

  HttpRequest(int server_fd, HttpResponse &httpResponse);
  ~HttpRequest();
  void handle_http_request();

  void set_status_code(int status);
  int get_status_code() const;
  void load_max_body_size();
  size_t get_max_body_size() const;

  bool add_header(std::string &key, std::string &value);
  bool is_in_headers(const std::string &key) const;
  std::string get_value_from_headers(const std::string &key) const;

  // リクエストの解析
  bool parse_http_request(const std::string &request, std::string &method,
                          std::string &path, std::string &version);
  ConfigMap get_best_match_config(const std::string &path);

  void clear();

private:
  HttpResponse &response;
  int status_code;
  std::string _root;
  size_t max_body_size;

  static const size_t k_default_max_body;

  void conf_init();
  std::map<int, std::string> extract_error_page_map(const std::vector<std::string>& tokens);
  void init_cgi_extensions();
  void init_file_index();
  void merge_config(ConfigMap &base, const ConfigMap &override);
  // GETの処理
  ResourceType get_resource_type(const std::string &path);
  void handle_get_request(std::string path);
  void handle_directory_request(std::string path);
  bool is_cgi_request(const std::string &path);
  void handle_cgi_request(const std::string &cgi_path);
  // POSTの処理
  void handle_post_request();
  bool is_location_upload_file(const std::string file_path);
  bool is_location_has_cgi();
  // DELETEの処理
  void handle_delete_request(const std::string path);
  int handle_directory_delete(const std::string &dir_path);
  int delete_all_directory_content(const std::string &dir_path);
  int handle_file_delete(const std::string &file_path);
  // autoindex (directory listing)
  void init_autoindex();
  std::string generate_directory_listing(const std::string &dir_path);
  // Utils
  std::string get_requested_resource(const std::string &path);
  void handle_file_request(const std::string &file_path);

  RedirStatus handle_redirection();

  HttpRequest(const HttpRequest &other);
  HttpRequest &operator=(const HttpRequest &other);

  void print_best_match_config() const;

  void handle_error(int status_code);
};
