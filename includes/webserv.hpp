#pragma once

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

class Server {
private:
    std::string public_root;  // 静的ファイルのルートディレクトリ
    int server_fd;
    struct sockaddr_in address;
    int port;

    void create_socket();
    void bind_socket();
    void listen_socket();
    void handle_client(int client_socket);

public:
    // Server(int port);
    Server(int port, const std::string& root);
    ~Server();
    void run();
};


std::string read_file(const std::string& file_path);
