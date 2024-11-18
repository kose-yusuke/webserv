#pragma once

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>

class Server {
private:
    std::string public_root;  // 静的ファイルのルートディレクトリ
    int server_fd;
    struct sockaddr_in address;
    int port;
    std::string error_404;

    void create_socket();
    void bind_socket();
    void listen_socket();
    void handle_client(int client_socket);

public:
    Server(const std::string& config_path);
    ~Server();
    void run();
};


std::string read_file(const std::string& file_path);
void send_custom_error_page(int client_socket, int status_code, const std::string& error_page);
std::map<std::string, std::string> parse_nginx_config(const std::string& config_path);