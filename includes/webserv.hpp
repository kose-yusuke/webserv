#pragma once

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <vector>
#include <thread>
#include <cstring>
#include <unistd.h>

class Server {
private:
    std::string public_root;
    int server_fd;
    struct sockaddr_in address;
    int port;
    std::string error_404;

    void create_socket();
    void bind_socket();
    void listen_socket();

    void handle_client(int client_socket);

    void handle_get_request(int client_socket, std::string path);
    void handle_post_request(int client_socket, const std::string& request);
    void handle_delete_request();

    bool parse_http_request(const std::string& request, std::string& method, std::string& path, std::string& version);
    void send_error_response(int client_socket, int status_code, const std::string& message);
    void send_custom_error_page(int client_socket, int status_code, const std::string& file_path);

public:
    Server();
    Server(const std::map<std::string, std::string>& config);
    ~Server();
    Server(const Server &src);
    Server& operator=(const Server &src);

    void run();
};

std::string read_file(const std::string& file_path);
int print_error_message(const std::string& message);