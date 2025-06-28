/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:38 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/06/28 16:19:26 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ResponseTypes.hpp"
#include "Utils.hpp"
#include "types.hpp"
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
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

class HttpResponse;
class VirtualHostRouter;
class CgiSession;
class CgiParser;

enum ResourceType { File, Directory, NotFound };
enum RedirStatus { REDIR_NONE, REDIR_SUCCESS, REDIR_FAILED };
enum HttpStatus {};

class HttpRequest {
public:
  // メンバ変数（仮）
  std::string method_;
  std::string path_;
  std::string version_;
  HeaderMap headers_;
  std::vector<char> body_data_;


  bool is_autoindex_enabled_;
  std::string index_file_name_;
  std::vector<std::string> cgi_extensions_;
  std::vector<std::string> allow_methods_;
  std::map<int, std::string> error_page_map_;

  ConfigMap server_config_;
  LocationMap location_configs_;
  ConfigMap best_match_config_;

  HttpRequest(int fd, const VirtualHostRouter *router, HttpResponse &httpResponse);
  ~HttpRequest();

  void handle_http_request();

  ConnectionPolicy get_connection_policy() const;
  void set_connection_policy(ConnectionPolicy policy);

  const std::string &get_method() const { return method_; }
  const std::string &get_path() const { return path_; }
  const std::vector<char> &get_body() const { return body_data_; }

  void set_status_code(int status);
  int get_status_code() const;
  void load_max_body_size();
  size_t get_max_body_size() const;

  const std::string &get_header_value(const std::string &key) const;
  const std::vector<std::string> &
  get_header_values(const std::string &key) const;
  void add_header(const std::string &key, const std::string &value);
  bool is_in_headers(const std::string &key) const;

  // リクエストの解析
  ConfigMap get_best_match_config(const std::string &path);

  void clear();

  bool has_cgi_session() const;
  CgiSession *get_cgi_session() const;
  void clear_cgi_session();

private:
  int client_fd_;
  HttpResponse &response_;
  const VirtualHostRouter *virtual_host_router_;
  CgiSession *cgi_session_;
  CgiParser *cgi_parser_;
  ConnectionPolicy connection_policy_;
  int status_code_;
  std::string _root;
  size_t max_body_size_;

  static const size_t k_default_max_body_;

  void select_server_by_host();
  void conf_init();
  std::map<int, std::string>
  extract_error_page_map(const std::vector<std::string> &tokens);
  void init_cgi_extensions();
  void init_file_index();
  void merge_config(ConfigMap &base, const ConfigMap &override);
  // GETの処理
  ResourceType get_resource_type(const std::string &path);
  void handle_get_request(std::string path);
  void handle_directory_request(std::string path);
  // POSTの処理
  void handle_post_request();
  bool is_location_upload_file(const std::string file_path);
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
  void launch_cgi(const std::string &cgi_path);

  bool validate_client_body_size();

  HttpRequest(const HttpRequest &other);
  HttpRequest &operator=(const HttpRequest &other);

  void handle_error(int status_code);
};
