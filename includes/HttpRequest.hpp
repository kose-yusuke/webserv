/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:38 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/05 18:45:43 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <unistd.h>
#include <sys/stat.h>

enum ResourceType {
    File,
    Directory,
    NotFound
};

class HttpRequest {
public:
    // メンバ変数（仮）
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    std::map<std::string, std::string> headers; 
    bool is_autoindex_enabled;

    HttpRequest() {};
    HttpRequest(const std::map<std::string, std::vector<std::string> >& config);
    void handleHttpRequest(int clientFd, const char *buffer, int nbytes);
    // リクエストの解析
    bool parse_http_request(const std::string &request, std::string &method, std::string &path, std::string &version);
    ResourceType get_resource_type(const std::string &path);
    // GET, POST, DELETE の処理
    void handle_get_request(int client_socket, std::string path);
    void handle_directory_request(int client_socket, std::string path);
    void handle_post_request(int client_socket, const std::string &request);
    void handle_delete_request();
    // std::string read_file(const std::string &file_path);

    private:
        std::string get_requested_resource(const std::string &path);
        void handle_file_request(int client_socket, const std::string &file_path);
};


// utils
bool file_exists(const std::string &path);
bool ends_with(const std::string &str, const std::string &suffix);
bool has_index_file(const std::string &dir_path);
bool is_directory(const std::string &path);