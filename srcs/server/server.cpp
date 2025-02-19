/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/20 01:09:37 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/webserv.hpp"
#include "../config/config_parse.hpp"

Server::Server(){}

Server::Server(const std::map<std::string, std::string>& config)
{    
    try {
        std::stringstream ss(config.find("listen")->second);
        if (!(ss >> port)) {  
            throw std::runtime_error("Invalid port number: " + config.find("listen")->second);
        }
        public_root = config.find("root")->second;
        std::map<std::string, std::string>::const_iterator it = config.find("error_page 404");
        error_404 = (it != config.end()) ? it->second : "404.html";

        create_socket();
        bind_socket();
        listen_socket();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error initializing server: ") + e.what());
    }
}

// Server::~Server() {
//     close(server_fd);
// }

// Server::~Server() {
//     std::cout << "Closing server_fd: " << server_fd << " (port: " << port << ")\n";
//     close(server_fd);
// }

Server::~Server() {
    if (server_fd >= 0) {  // ✅ 無効な `server_fd` は閉じない
        std::cout << "Closing server_fd: " << server_fd << " (port: " << port << ")\n";
        close(server_fd);
    }
}



Server::Server(const Server &src)
{
    port = src.port;
    public_root = src.public_root;
    error_404 = src.error_404;
    server_fd = -1;
    address = src.address;
}

Server& Server::operator=(const Server &src)
{
    if (this != &src)
    {
        port = src.port;
        public_root = src.public_root;
        error_404 = src.error_404;
        server_fd = -1;
        address = src.address;
    }
    return (*this);
}

void Server::create_socket() 
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        throw std::runtime_error("Socket creation failed");
    }
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        throw std::runtime_error("Failed to set socket options");
    }
    std::cout << "Socket created and options set successfully\n";
}

void Server::bind_socket() 
{
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
    std::cout << "Socket bound to port " << port << "\n";
}

void Server::listen_socket() 
{
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("Listen failed");
        close(server_fd);
        throw std::runtime_error("Failed to listen on socket");
    }
    std::cout << "Server is listening on port " << port << " (server_fd: " << server_fd << ")\n";
}

void Server::send_custom_error_page(int client_socket, int status_code, const std::string& error_page) 
{
    try {
        std::string file_content = read_file("./public/" + error_page);

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

// Request/Responseは分離する

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

// ファイルを保存する
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

// ファイルを削除する
void Server::handle_delete_request() {}


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
    } else if (method == "DELETE") {
        handle_delete_request();
    } else {
        send_error_response(client_socket, 405, "Method Not Allowed");
    }

    close(client_socket);
}

// void Server::run() 
// {
//     std::cout << "Server FD at run start: " << server_fd << " (port: " << port << ")\n";
    
//     if (server_fd < 0) {
//         std::cerr << "Error: Server socket is invalid (Bad file descriptor)\n";
//         return;
//     }
    // while (true) 
    // {
    //     int addrlen = sizeof(address);
    //     int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    //     if (client_socket < 0) {
    //         std::cerr << "Failed to accept connection\n";
    //         perror("accept failed");
    //         continue;
    //     }
    //     handle_client(client_socket);
    // }
// }

void Server::run() 
{
    int n = 1;
    std::cout << "Server FD at run start: " << server_fd << " (port: " << port << ")\n";
    
    if (server_fd < 0) {
        std::cerr << "Error: Server socket is invalid (Bad file descriptor)\n";
        return;
    }

    while (n > 0) 
    {
        int addrlen = sizeof(address);
        
        std::cout << "Before accept: server_fd = " << server_fd << " (port: " << port << ")\n";
        
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        if (client_socket < 0) {
            perror("accept failed");
            std::cerr << "accept() error on port " << port << ": " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "Accepted connection on port " << port << " (server_fd: " << server_fd << ")\n";
        handle_client(client_socket);
        n--;
    }
}
