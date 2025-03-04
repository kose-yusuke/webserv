/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:38 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/02 17:31:08 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <map>

class HttpRequest {
public:
    // メンバ変数（仮）
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    std::map<std::string, std::string> headers; 

    void handleHttpRequest(int clientFd, const char *buffer, int nbytes);
    
    // リクエストの解析
    bool parse_http_request(const std::string &request, std::string &method, std::string &path, std::string &version);
    
    // GET, POST, DELETE の処理
    void handle_get_request(int client_socket, std::string path);
    void handle_post_request(int client_socket, const std::string &request);
    void handle_delete_request();
    std::string read_file(const std::string &file_path);
};
