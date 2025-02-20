/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/20 22:23:35 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../includes/webserv.hpp"
#include "../config/config_parse.hpp"

Server::Server(){}

Server::Server(const std::map<std::string, std::vector<std::string> >& config)
    : config(config)
{    
    try {
        // listen
        std::map<std::string, std::vector<std::string> >::const_iterator listen_it = config.find("listen");
        if (listen_it == config.end() || listen_it->second.empty())
            throw std::runtime_error("Missing required key: listen");

        for (size_t i = 0; i < listen_it->second.size(); i++) {
            int port;
            std::stringstream ss(listen_it->second[i]);
            if (!(ss >> port))
                throw std::runtime_error("Invalid port number: " + listen_it->second[i]);
            listen_ports.push_back(port);
        }

        // root
        std::map<std::string, std::vector<std::string> >::const_iterator root_it = config.find("root");
        if (root_it == config.end() || root_it->second.empty()) {
            throw std::runtime_error("Missing required key: root");
        }
        public_root = root_it->second[0];
        
        std::map<std::string, std::vector<std::string> >::const_iterator error_it = config.find("error_page 404");
        error_404 = (error_it != config.end() && !error_it->second.empty()) ? error_it->second[0] : "404.html";


        create_sockets();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error initializing server: ") + e.what());
    }
}

void Server::create_sockets() {
    std::vector<int> temp_fds;

    try {
        for (size_t i = 0; i < listen_ports.size(); i++) {
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd == -1) {
                throw std::runtime_error("Socket creation failed for port: " + std::to_string(listen_ports[i]));
            }

            int opt = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
                close(sockfd);
                throw std::runtime_error("Failed to set socket options for port: " + std::to_string(listen_ports[i]));
            }

            bind_socket(sockfd, listen_ports[i]);
            listen_socket(sockfd, listen_ports[i]);

            temp_fds.push_back(sockfd);
        }

        server_fds = temp_fds;
        std::cout << "Socket created and options set successfully\n";

    } catch (...) {
        for (size_t i = 0; i < temp_fds.size(); i++) {
            close(temp_fds[i]);
        }
        throw;
    }
}


void Server::bind_socket(int sockfd, int port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("Failed to bind socket to port: " + std::to_string(port));
    addresses.push_back(addr);
    std::cout << "Socket bound to port " << port << "\n";
}

void Server::listen_socket(int sockfd, int port) {
    if (listen(sockfd, SOMAXCONN) < 0) {
        close(sockfd);
        throw std::runtime_error("Failed to listen on port: " + std::to_string(port));
    }
    std::cout << "Server is listening on port " << port << " (fd: " << sockfd << ")\n";
}

Server::~Server() {
    for (size_t i = 0; i < server_fds.size(); i++) {
        close(server_fds[i]);
        std::cout << "Closed server_fd: " << server_fds[i] << std::endl;
        server_fds[i] = -1;
    }
}

// Server::Server(const Server &src)
//     : config(src.config), listen_ports(src.listen_ports), public_root(src.public_root),
//       error_404(src.error_404), server_fds()
// {
//     try {
//         create_sockets();  // コピー時に新しくソケットを作る
//     } catch (const std::exception& e) {
//         throw std::runtime_error("Error copying server: " + std::string(e.what()));
//     }
// }

// Server& Server::operator=(const Server &src)
// {
//     if (this != &src)
//     {
//         config = src.config;
//         listen_ports = src.listen_ports;
//         public_root = src.public_root;
//         error_404 = src.error_404;

//         for (size_t i = 0; i < server_fds.size(); i++) {
//             close(server_fds[i]);
//         }
//         server_fds.clear();

//         try {
//             create_sockets();
//         } catch (const std::exception& e) {
//             throw std::runtime_error("Error copying server: " + std::string(e.what()));
//         }
//     }
//     return *this;
// }

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

void Server::run() 
{   
    while (true) 
    {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        // server_fdsを read_fds に登録。
        int max_fd = -1;
        for (size_t i = 0; i < server_fds.size(); i++) {
            FD_SET(server_fds[i], &read_fds);
            if (server_fds[i] > max_fd) {
                max_fd = server_fds[i];
            }
        }

        std::cout << "Waiting for connections...\n";
        // server_fd に接続があるかどうかチェック
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            std::cerr << "Error: select() failed with errno " << errno << " (" << strerror(errno) << ")\n";
            continue;
        }

        // どのポートに接続があったかをFD_ISSET()でチェック
        for (size_t i = 0; i < server_fds.size(); i++) {
            if (FD_ISSET(server_fds[i], &read_fds)) {
                int addrlen = sizeof(addresses[i]);
                int client_socket = accept(server_fds[i], (struct sockaddr*)&addresses[i], (socklen_t*)&addrlen);

                if (client_socket < 0) {
                    perror("accept failed");
                    std::cerr << "accept() error on port " << listen_ports[i] << ": " << strerror(errno) << std::endl;
                    continue;
                }

                std::cout << "Accepted connection on port " << listen_ports[i] << " (server_fd: " << server_fds[i] << ")\n";
                handle_client(client_socket);
            }
        }
    }
}
