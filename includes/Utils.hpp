/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/05 21:09:37 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/05 21:09:39 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <stdexcept>
#include <string>
// #include <thread>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#define LOG_DEBUG_FUNC() log(LOG_FUNC, std::string(__func__) + "() called")
#define LOG_DEBUG_FUNC_FD(fd)                                                  \
  logfd(LOG_FUNC, std::string(__func__) + "() called on fd: ", fd)

// kosekiさんから引き継いだ時点でのServer classのheader（一旦コメントアウト）
// class Server {
// private:
//     std::map<std::string, std::vector<std::string> > config;
//     std::vector<int> listen_ports;
//     std::string public_root;
//     std::string error_404;
//     std::vector<int> server_fds;
//     std::vector<sockaddr_in> addresses;

//     void create_sockets();
//     void bind_socket(int sockfd, int port);
//     void listen_socket(int sockfd, int port);

//     void handle_client(int client_socket);

//     void handle_get_request(int client_socket, std::string path);
//     void handle_post_request(int client_socket, const std::string& request);
//     void handle_delete_request();

//     bool parse_http_request(const std::string& request, std::string& method,
//     std::string& path, std::string& version); void send_error_response(int
//     client_socket, int status_code, const std::string& message); void
//     send_custom_error_page(int client_socket, int status_code, const
//     std::string& file_path);

// public:
//     Server();
//     Server(const std::map<std::string, std::vector<std::string> >& config);
//     ~Server();
//     // Server(const Server &src);
//     // Server& operator=(const Server &src);
//     void run();
// };

std::string read_file(const std::string &file_path);
int print_error_message(const std::string &message);

// utils
bool file_exists(const std::string &path);
bool ends_with(const std::string &str, const std::string &suffix);
bool has_index_file(const std::string &dir_path);
bool is_directory(const std::string &path);

// log
enum LogLevel { LOG_FUNC, LOG_INFO, LOG_DEBUG, LOG_WARNING, LOG_ERROR };
// global 変数宣言
extern LogLevel current_log_level;
extern std::ofstream debug_log;

void log(LogLevel level, const std::string &message);
void logfd(LogLevel level, const std::string &prefix, int fd);

// trim string
std::string trim_left(const std::string &s);
std::string trim_right(const std::string &s);
std::string trim(const std::string &s);

// convert string to number
size_t convert_str_to_size(const std::string &num_str);
size_t parse_hex(const std::string &s);
