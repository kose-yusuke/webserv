/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:05 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/12 16:41:09 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include <iostream>

HttpRequest::HttpRequest() {}

size_t HttpRequest::get_content_length() { return 0; };
bool HttpRequest::is_header_received() { return true; };
void HttpRequest::parse_header(const std::string &request) {
  std::cout << "Http request received:\n";
  std::cout << request << std::endl;
};
void HttpRequest::parse_body(const std::string &request) { (void)request; };
void HttpRequest::clear() {};

void HttpRequest::handleHttpRequest(int clientFd, const char *buffer,
                                    int nbytes) {
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

bool HttpRequest::parse_http_request(const std::string &request,
                                     std::string &method, std::string &path,
                                     std::string &version) {
  std::istringstream request_stream(request);
  if (!(request_stream >> method >> path >> version)) {
    return false;
  }
  return true;
}

void HttpRequest::handle_get_request(int client_socket, std::string path) {
  if (path == "/") {
    path = "/index.html";
  }

  std::string file_path = "./public" + path;
  try {
    std::cout << file_path << std::endl;
    std::string file_content = read_file(file_path);
    HttpResponse::send_response(client_socket, 200, file_content, "text/html");
  } catch (const std::exception &e) {
    HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
  }
}

void HttpRequest::handle_post_request(int client_socket,
                                      const std::string &request) {
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

std::string HttpRequest::read_file(const std::string &file_path) {
  std::ifstream file(file_path.c_str(), std::ios::in);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + file_path);
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}
