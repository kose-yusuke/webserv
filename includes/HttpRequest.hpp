/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:38 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/09 15:46:24 by koseki.yusu      ###   ########.fr       */
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
#include <sys/wait.h>
#include <sys/types.h>

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
    std::vector<std::string> cgi_extensions;
    std::vector<std::string> allow_methods;

    HttpRequest() {};
    HttpRequest(const std::map<std::string, std::vector<std::string> >& config);
    void handleHttpRequest(int clientFd, const char *buffer, int nbytes);
    // リクエストの解析
    bool parse_http_request(const std::string &request, std::string &method, std::string &path, std::string &version);
    // GETの処理
    ResourceType get_resource_type(const std::string &path);
    void handle_get_request(int client_socket, std::string path);
    void handle_directory_request(int client_socket, std::string path);
    bool is_cgi_request(const std::string& path);
    void handle_cgi_request(int client_socket, const std::string& cgi_path);
    // POSTの処理
    void handle_post_request(int client_socket, const std::string &request, std::string path);
    // DELETEの処理
    void handle_delete_request(int client_socket,std::string path);
    int handle_file_delete(const std::string& file_path);
    int delete_directory(const std::string& dir_path);

    private:
        std::string get_requested_resource(const std::string &path);
        void handle_file_request(int client_socket, const std::string &file_path);
};
