/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/15 18:07:32 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/webserv.hpp"

Server::Server(const std::string& config_path)
{
    std::map<std::string, std::string> config = parse_nginx_config(config_path);

    port = std::stoi(config["listen"]);
    public_root = config["root"];
    error_404 = config["error_page 404"];
    create_socket();
    bind_socket();
    listen_socket();
}

Server::~Server() {
    close(server_fd);
}

void Server::create_socket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        throw std::runtime_error("Socket creation failed");
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        throw std::runtime_error("Failed to set socket options");
    }
    std::cout << "Socket created and options set successfully\n";
}

void Server::bind_socket() {
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
    std::cout << "Socket bound to port " << port << "\n";
}

void Server::listen_socket() {
    if (listen(server_fd, 3) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
    std::cout << "Server is listening on port " << port << "\n";
}

void Server::send_custom_error_page(int client_socket, int status_code, const std::string& error_page) 
{
    try {
        std::string file_content = read_file("./srcs/public/" + error_page);

        std::ostringstream response;
        response << "HTTP/1.1 " << status_code << " ";
        if (status_code == 404) {
            response << "Not Found";
        } else if (status_code == 405) {
            response << "Method Not Allowed";
        }
        response << "\r\n";
        response << "Content-Length: " << file_content.size() << "\r\n";
        response << "Content-Type: text/html\r\n\r\n";
        response << file_content;

        send(client_socket, response.str().c_str(), response.str().size(), 0);
    } catch (const std::exception& e) {
        std::ostringstream fallback;
        fallback << "HTTP/1.1 " << status_code << " ";
        if (status_code == 404) {
            fallback << "Not Found";
        } else if (status_code == 405) {
            fallback << "Method Not Allowed";
        }
        fallback << "\r\nContent-Length: 9\r\n\r\nNot Found";
        send(client_socket, fallback.str().c_str(), fallback.str().size(), 0);
    }
}

bool Server::parse_http_request(const std::string& request, std::string& method, std::string& path, std::string& version) 
{
    std::istringstream request_stream(request);
    if (!(request_stream >> method >> path >> version)) {
        return false;
    }
    return true;
}

void Server::handle_get_request(int client_socket, std::string path) 
{
    if (path == "/") {
        path = "/index.html";
    }

    std::string file_path = public_root + path;
    try {
        std::cout << file_path << std::endl;
        std::string file_content = read_file(file_path);
        // std::string mime_type = get_mime_type(file_path);

        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Length: " << file_content.size() << "\r\n";
        // response << "Content-Type: " << mime_type << "\r\n\r\n";
        response << "Content-Type: text/html\r\n\r\n";
        response << file_content;

        send(client_socket, response.str().c_str(), response.str().size(), 0);

    } catch (const std::exception& e) {
        send_custom_error_page(client_socket, 404, error_404);
    }
}

void Server::handle_post_request(int client_socket, const std::string& request) {
    size_t body_start = request.find("\r\n\r\n");
    if (body_start == std::string::npos) {
        send_error_response(client_socket, 400, "Bad Request");
        return;
    }

    std::string body = request.substr(body_start + 4);  // ボディ部分を抽出
    std::cout << "Received POST body: " << body << "\n";

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Length: " << body.size() << "\r\n";
    response << "Content-Type: text/plain\r\n\r\n";
    response << body;

    send(client_socket, response.str().c_str(), response.str().size(), 0);
}

void Server::send_error_response(int client_socket, int status_code, const std::string& message) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
    response << "Content-Length: " << message.size() << "\r\n";
    response << "Content-Type: text/plain\r\n\r\n";
    response << message;

    send(client_socket, response.str().c_str(), response.str().size(), 0);
}


void Server::handle_client(int client_socket) {
    char buffer[1024] = {0};
    int valread = read(client_socket, buffer, sizeof(buffer));
    if (valread <= 0) {
        close(client_socket);
        return;
    }

    std::string method, path, version;
    if (!parse_http_request(buffer, method, path, version)) {
        send_error_response(client_socket, 400, "Bad Request");
        close(client_socket);
        return;
    }

    std::cout << "HTTP Method: " << method << ", Path: " << path << "\n";

    if (method == "GET") {
        handle_get_request(client_socket, path);
    } else if (method == "POST") {
        handle_post_request(client_socket, buffer);
    } else {
        send_error_response(client_socket, 405, "Method Not Allowed");
    }

    close(client_socket);
}

void Server::run() 
{
    while (true) 
    {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }
        handle_client(client_socket);
    }
}
