/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:05 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/05 18:45:02 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

HttpRequest::HttpRequest(const std::map<std::string, std::vector<std::string> >& config) {
    // デフォルトは autoindex OFF
    is_autoindex_enabled = false;

    std::map<std::string, std::vector<std::string> >::const_iterator it = config.find("autoindex");
    if (it != config.end() && !it->second.empty() && it->second[0] == "on") {
        is_autoindex_enabled = true;
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

    if (method == "GET") {
        handle_get_request(clientFd, path);
    } else if (method == "POST") {
        handle_post_request(clientFd, buffer);
    } else if (method == "DELETE") {
        handle_delete_request();
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

// flow図における get_requested_resource() ?
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
        handle_file_request(client_socket, file_path);
    } else {
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

    // `index.html` が存在するか確認
    if (has_index_file("./public" + path)) {
        handle_file_request(client_socket, "./public" + path + "index.html");
    } else {
        // autoindexがONの場合、ディレクトリリストを生成する
        if (is_autoindex_enabled) {
            // std::string dir_listing = generate_directory_listing(path);
            std::string dir_listing = "test";
            HttpResponse::send_response(client_socket, 200, dir_listing, "text/html");
        } else {
            // 403 forbidden
            HttpResponse::send_custom_error_page(client_socket, 403, "403.html");
        }
    }
}

bool ends_with(const std::string &str, const std::string &suffix) {
    if (str.length() < suffix.length()) return false;
    return (str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0);
}

bool file_exists(const std::string &path) {
    return (access(path.c_str(), F_OK) == 0);
}

bool has_index_file(const std::string &dir_path) {
    std::string index_file = dir_path + "/index.html";
    return file_exists(index_file);
}

bool is_directory(const std::string &path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    return S_ISDIR(buffer.st_mode);
}


void HttpRequest::handle_post_request(int client_socket, const std::string &request) {
    size_t body_start = request.find("\r\n\r\n");
    if (body_start == std::string::npos) {
        HttpResponse::send_error_response(client_socket, 400, "Bad Request");
        return;
    }

    std::string body = request.substr(body_start + 4);
    std::cout << "Received POST body: " << body << "\n";

    HttpResponse::send_response(client_socket, 200, body, "text/plain");
}

void HttpRequest::handle_delete_request() {
    // DELETE メソッドの処理（未実装）
}

// autoindex

// std::string generate_directory_listing(const std::string &dir_path) {
    
// }