/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:05 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/11 14:37:35 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Utils.hpp"

HttpRequest::HttpRequest(const std::map<std::string, std::vector<std::string> >& config) {
    // autoindex初期化
    is_autoindex_enabled = false;

    std::map<std::string, std::vector<std::string> >::const_iterator auto_it = config.find("autoindex");
    if (auto_it != config.end() && !auto_it->second.empty() && auto_it->second[0] == "on") {
        is_autoindex_enabled = true;
    }

    // cgi_extensions初期化
    // std::vector<std::string> cgi_extensions = {".cgi", ".php", ".py", ".pl"};
    std::map<std::string, std::vector<std::string> >::const_iterator cgi_it = config.find("cgi_extensions");
    if (cgi_it != config.end()) {
        cgi_extensions = cgi_it->second;
    }

    // allow_methods初期化
    std::map<std::string, std::vector<std::string> >::const_iterator it = config.find("allow_methods");
    if (it != config.end()) {
        allow_methods = it->second;
    } else {
        allow_methods.push_back("GET");
        allow_methods.push_back("POST");
        allow_methods.push_back("DELETE");
    }
}

void HttpRequest::handleHttpRequest(int clientFd, const char *buffer, int nbytes) {
    (void)nbytes;
    std::string method, path, version;
    if (!parse_http_request(buffer, method, path, version)) {
        HttpResponse::send_error_response(clientFd, 400, "Bad Request");
        return;
    }

    std::cout << "HTTP Method: " << method << ", Path: " << path << "\n";

    if (std::find(allow_methods.begin(), allow_methods.end(), method) != allow_methods.end()) {
        if (method == "GET") {
            handle_get_request(clientFd, path);
        } else if (method == "POST") {
            handle_post_request(clientFd, buffer, path);
        } else if (method == "DELETE") {
            handle_delete_request(clientFd,path);
        }
    } else {
        HttpResponse::send_error_response(clientFd, 405, "Method Not Allowed");
    }
}

bool HttpRequest::parse_http_request(const std::string &request, std::string &method, std::string &path, std::string &version) {
    std::istringstream request_stream(request);
    if (!(request_stream >> method >> path >> version)) {
        return false;
    }
    return true;
}

/*GET Request*/
void HttpRequest::handle_get_request(int client_socket, std::string path) {

    std::string file_path = get_requested_resource(path);
    
    if (file_path.empty()) {
        HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
        return;
    }
    
    ResourceType type = get_resource_type(file_path);

    if (type == Directory) {
        handle_directory_request(client_socket, path);
    } else if (type == File) {
        if (is_cgi_request(file_path))
            handle_cgi_request(client_socket, file_path);
        else
            handle_file_request(client_socket, file_path);
    }
    else {
        HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
    }
}

std::string HttpRequest::get_requested_resource(const std::string& path) {
    std::string root = "./public";
    std::string file_path = root + path;

    if (!file_exists(file_path)) {
        return "";
    }
    return file_path;
}

ResourceType HttpRequest::get_resource_type(const std::string& path) {
    if (is_directory(path)) {
        return Directory;
    } else if (file_exists(path)) {
        return File;
    }
    return NotFound;
}

/*Requestがディレクトリかファイルかの分岐処理*/

void HttpRequest::handle_file_request(int client_socket, const std::string& file_path) {
    std::ifstream file(file_path.c_str(), std::ios::in);
    if (!file.is_open()) {
        HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string file_content = buffer.str();
    HttpResponse::send_response(client_socket, 200, file_content, "text/html");
}

void HttpRequest::handle_directory_request(int client_socket, std::string path) {
    // URLの末尾に `/` がない場合、リダイレクト（301）
    if (!ends_with(path, "/")) {
        std::string new_location = path + "/";
        HttpResponse::send_redirect(client_socket, 301, new_location);
        return;
    }

    // `index.html` が存在するか確認 - 本当はこの辺の　public になっているところはrootとかで置き換える必要あり
    if (has_index_file("./public" + path)) {
        handle_file_request(client_socket, "./public" + path + "index.html");
    } else {
        // autoindexがONの場合、ディレクトリリストを生成する
        if (is_autoindex_enabled) {
            std::string dir_listing = generate_directory_listing("./public" + path);
            HttpResponse::send_response(client_socket, 200, dir_listing, "text/html");
        } else {
            // 403 forbidden
            HttpResponse::send_custom_error_page(client_socket, 403, "403.html");
        }
    }
}

/*Cgi関連の処理*/

bool HttpRequest::is_cgi_request(const std::string& path) {
    std::string::size_type dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return false;
    }

    std::string extension = path.substr(dot_pos);
    for (size_t i = 0; i < cgi_extensions.size(); ++i) {
        if (cgi_extensions[i] == extension) {
            return true;
        }
    }
    return false;
    // return (extension == ".cgi" || extension == ".php" || extension == ".py" || extension == ".pl"); → confファイルで指示あり？
}

void HttpRequest::handle_cgi_request(int client_socket, const std::string& cgi_path) {
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        std::cerr << "pipe failed" << std::endl;
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "pipe failed" << std::endl;
        exit(1);
    }

    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        char* argv[2];
        argv[0] = const_cast<char*>(cgi_path.c_str());
        argv[1] = NULL; 
        char* envp[] = {NULL};
        execve(cgi_path.c_str(), argv, envp);
        exit(1);
    }

    close(pipefd[1]);
    char buffer[1024];
    std::string cgi_output;
    ssize_t bytes_read;
    
    while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        cgi_output += buffer;
    }

    close(pipefd[0]);
    waitpid(pid, NULL, 0);

    HttpResponse::send_response(client_socket, 200, cgi_output, "text/html");
}

// 何を書くべきかわからず未実装
bool is_location_upload_file()
{
    return true;
}

void HttpRequest::handle_post_request(int client_socket, const std::string &request, std::string path) {
    if (is_location_upload_file())
    {
        size_t body_start = request.find("\r\n\r\n");
        if (body_start == std::string::npos) {
            HttpResponse::send_error_response(client_socket, 400, "Bad Request");
            return;
        }

        std::string body = request.substr(body_start + 4);
        std::cout << "Received POST body: " << body << "\n";

        HttpResponse::send_response(client_socket, 201, body, "text/plain");   
    }
    else
    {
        // アップロードできない場合は通常のresource取得になるらしい (GETと同様処理)
        handle_get_request(client_socket,path);
    }
}

void HttpRequest::handle_delete_request(int client_socket, std::string path) {
    std::string file_path = get_requested_resource(path);
    
    if (file_path.empty()) {
        HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
        return;
    }
    
    ResourceType type = get_resource_type(file_path);

    // 書き込み権限
    if (access(file_path.c_str(), W_OK) != 0) {
        HttpResponse::send_error_response(client_socket, 403, "Forbidden");
        return;
    }

    int status = -1;
    if (type == Directory) {
        delete_directory(file_path);
    } else if (type == File) {
        if (is_cgi_request(file_path))
            handle_cgi_request(client_socket, file_path);
        else
            handle_file_delete(file_path);
    }
    else {
        HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
    }

    if (status == 0) {
        HttpResponse::send_response(client_socket, 204, "", "text/plain"); // 成功
    } else {
        HttpResponse::send_error_response(client_socket, 500, "Internal Server Error"); // 失敗
    }
}

int HttpRequest::handle_file_delete(const std::string& file_path)
{
    std::string html = file_path;
    return 0;
}

int HttpRequest::delete_directory(const std::string& dir_path) {
    std::string html = dir_path;
    return 0;
}

std::string HttpRequest::generate_directory_listing(const std::string &dir_path) {
    DIR *dir = opendir(dir_path.c_str());
    if (!dir) {
        return "<html><body><h1>403 Forbidden</h1></body></html>";
    }
    
    std::ostringstream html;
    html << "<html><head><title>Index of " << dir_path << "</title></head>";
    html << "<body><h1>Index of " << dir_path << "</h1>";
    html << "<ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
    }
    html << "</ul></body></html>";
    closedir(dir);
    
    return html.str();
}