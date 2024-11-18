#pragma once

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <cstring>
#include <unistd.h>

class Server {
private:
    std::string public_root;  // 静的ファイルのルートディレクトリ
    int server_fd;
    struct sockaddr_in address;
    int port;

    // エラーページ
    std::string error_404;

    // サーバー動作のためのメソッド
    void create_socket();
    void bind_socket();
    void listen_socket();

    // クライアントリクエスト処理
    void handle_client(int client_socket);

    // 個別のメソッド処理
    void handle_get_request(int client_socket, std::string path);
    void handle_post_request(int client_socket, const std::string& request);

    // 共通処理
    bool parse_http_request(const std::string& request, std::string& method, std::string& path, std::string& version);
    void send_error_response(int client_socket, int status_code, const std::string& message);
    void send_custom_error_page(int client_socket, int status_code, const std::string& file_path);

public:
    Server(const std::string& config_path);
    ~Server();

    // サーバーの実行
    void run();
};

std::string read_file(const std::string& file_path);
std::map<std::string, std::string> parse_nginx_config(const std::string& config_path);