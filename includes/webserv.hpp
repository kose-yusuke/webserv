#pragma once

#include <iostream>
#include <string>
#include <netinet/in.h>

class Server {
private:
    int server_fd;
    struct sockaddr_in address;
    int port;

    void create_socket();
    void bind_socket();
    void listen_socket();
    void handle_client(int client_socket);

public:
    Server(int port);
    ~Server();
    void run();
};

