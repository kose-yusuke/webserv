/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:38 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/15 03:32:26 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>

enum ResourceType {
    File,
    Directory,
    NotFound
};

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

    MethodType methodType_;
    bool is_autoindex_enabled;
    std::vector<std::string> cgi_extensions;
    std::vector<std::string> allow_methods;
    std::map<std::string, std::vector<std::string> >  server_configs;
    std::map<std::string, std::map<std::string, std::vector<std::string> > > location_configs;
    std::map<std::string, std::vector<std::string> > best_match_location_config;

    HttpRequest() {};
    HttpRequest(const std::map<std::string, std::vector<std::string> >& config, const std::map<std::string, std::map<std::string, std::vector<std::string> > >&  location_config);
    void handleHttpRequest(int clientFd, const char *buffer, int nbytes);
    // リクエストの解析
    bool parse_http_request(const std::string &request, std::string &method, std::string &path, std::string &version);
    std::map<std::string, std::vector<std::string> > get_location_config(const std::string& path);

    // GETの処理
    ResourceType get_resource_type(const std::string &path);
    void handle_get_request(int client_socket, std::string path);
    void handle_directory_request(int client_socket, std::string path);
    bool is_cgi_request(const std::string& path);
    void handle_cgi_request(int client_socket, const std::string& cgi_path);
    // POSTの処理
    void handle_post_request(int client_socket, const std::string &request, std::string path);
    bool is_location_upload_file(int client_socket, const std::string file_path);
    // DELETEの処理
    void handle_delete_request(int client_socket,std::string path);
    int handle_file_delete(const std::string& file_path);
    int delete_directory(const std::string& dir_path);
    // autoindex (directory listing)
    std::string generate_directory_listing(const std::string &dir_path);

    private:
        std::string _root;

        std::string get_requested_resource(const std::string &path);
        void handle_file_request(int client_socket, const std::string &file_path);
};
