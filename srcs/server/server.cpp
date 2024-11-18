#include "../../includes/webserv.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sstream>
#include <stdexcept>

Server::Server(int port, const std::string& root) : public_root(root), port(port) {
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

void Server::handle_client(int client_socket) {
    char buffer[1024] = {0};
    int valread = read(client_socket, buffer, sizeof(buffer));
    if (valread <= 0) {
        close(client_socket);
        return;
    }

    std::string request_line(buffer);
    std::istringstream request_stream(request_line);
    std::string method, path, version;
    if (request_stream >> method >> path >> version) 
    {
        std::cout << "HTTP Method: " << method << ", Path: " << path << "\n";

        // 静的ファイル提供ロジック
        if (method == "GET") {
            if (path == "/") {
                path = "/index.html";  // デフォルトファイル
            }

            // パスをローカルのファイルパスに変換
            std::string file_path = public_root + path;
            try {
                std::string file_content = read_file(file_path);

                // MIMEタイプを取得
                // std::string mime_type = get_mime_type(file_path);

                // HTTPレスポンスの生成
                std::ostringstream response;
                response << "HTTP/1.1 200 OK\r\n";
                response << "Content-Length: " << file_content.size() << "\r\n";
                response << "Content-Type: text/html\r\n\r\n";
                // response << "Content-Type: " << mime_type << "\r\n\r\n";
                response << file_content;

                send(client_socket, response.str().c_str(), response.str().size(), 0);
            } catch (const std::exception& e) {
                // ファイルが存在しない場合、404エラー
                const char* not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
                send_custom_error_page(client_socket, 404, "404.html");
                send(client_socket, not_found, strlen(not_found), 0);
            }
        }
        else if (method == "POST") 
        {
            // リクエストボディの開始位置を探す
            std::string request(buffer);
            size_t body_start = request.find("\r\n\r\n");
            if (body_start != std::string::npos) {
                std::string body = request.substr(body_start + 4);  // ボディ部分を抽出

                // ボディをログに表示
                std::cout << "Received POST body: " << body << "\n";

                // レスポンスを生成
                std::ostringstream response;
                response << "HTTP/1.1 200 OK\r\n";
                response << "Content-Length: " << body.size() << "\r\n";
                response << "Content-Type: text/plain\r\n\r\n";
                response << body;  // ボディをそのままレスポンスとして返す

                send(client_socket, response.str().c_str(), response.str().size(), 0);
            }
        }
        else 
        {
            // サポートされていないメソッド
            const char* not_allowed = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 18\r\n\r\nMethod Not Allowed";
            send(client_socket, not_allowed, strlen(not_allowed), 0);
        }
    }
    else 
    {
        // ボディが見つからない場合
        const char* bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Length: 11\r\n\r\nBad Request";
        send(client_socket, bad_request, strlen(bad_request), 0);
    }
    close(client_socket);
}

void Server::run() {
    while (true) {
        int addrlen = sizeof(address);
        int client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_socket < 0) {
            std::cerr << "Failed to accept connection\n";
            continue;
        }
        handle_client(client_socket);
    }
}
