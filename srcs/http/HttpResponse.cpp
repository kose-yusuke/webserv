/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/20 02:10:48 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include "Utils.hpp"

HttpResponse::HttpResponse() {}

HttpResponse::~HttpResponse() {}

bool HttpResponse::has_next_response() const { return !response_queue.empty(); }

std::string HttpResponse::get_next_response() {
  if (response_queue.empty()) {
    return "";
  }
  return response_queue.front();
}

void HttpResponse::pop_response() { response_queue.pop(); }

void HttpResponse::push_response(const std::string &response) {
  if (response.empty()) {
    return;
  }
  response_queue.push(response);
}

void HttpResponse::generate_response(int status_code,
                                     const std::string &content,
                                     const std::string &content_type) {
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " OK\r\n";
  response << "Content-Length: " << content.size() << "\r\n";
  response << "Content-Type: " << content_type << "\r\n\r\n";
  response << content;

  push_response(response.str());
  // send(client_socket, response.str().c_str(), response.str().size(), 0);
}

void HttpResponse::generate_custom_error_page(int status_code,
                                              const std::string &error_page) {
  try {
    std::string file_content = read_file("./public/" + error_page);

    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " ";
    if (status_code == 403) {
      response << "Forbidden";
    } else if (status_code == 404) {
      response << "Not Found";
    } else if (status_code == 405) {
      response << "Method Not Allowed";
    }
    response << "\r\n";
    response << "Content-Length: " << file_content.size() << "\r\n";
    response << "Content-Type: text/html\r\n\r\n";
    response << file_content;

    push_response(response.str());
    // send(client_socket, response.str().c_str(), response.str().size(), 0);
  } catch (const std::exception &e) {
    std::ostringstream fallback;
    fallback << "HTTP/1.1 " << status_code << " ";
    if (status_code == 404) {
      fallback << "Not Found";
    } else if (status_code == 405) {
      fallback << "Method Not Allowed";
    }
    fallback << "\r\nContent-Length: 9\r\n\r\nNot Found";

    push_response(fallback.str());
    // send(client_socket, fallback.str().c_str(), fallback.str().size(), 0);
  }
}

void HttpResponse::generate_error_response(int status_code,
                                           const std::string &message) {
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
  response << "Content-Length: " << message.size() << "\r\n";
  response << "Content-Type: text/plain\r\n\r\n";
  response << message;

  push_response(response.str());
  // send(client_socket, response.str().c_str(), response.str().size(), 0);
}

// 未実装
void HttpResponse::generate_redirect(int status_code,
                                     const std::string &new_location) {
  // std::cout << client_socket << std::endl;
  // std::cout << status_code << std::endl;
  // std::cout << new_location << std::endl;
  std::ostringstream response; // TODO: 仮の状態
  response << "HTTP/1.1 " << status_code << " ";
  response << HttpResponse::get_status_message(status_code) << "\r\n";
  response << "Location: " << new_location << "\r\n";
  response << "Content-Length: 0\r\n\r\n";

  push_response(response.str());
}

std::string HttpResponse::get_status_message(int status_code) {
  switch (status_code) {
  case 200:
    return "OK";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 500:
    return "Internal Server Error";
  default:
    return "Internal Server Error";
  }
}

HttpResponse::HttpResponse(const HttpResponse &other) { (void)other; }

HttpResponse &HttpResponse::operator=(const HttpResponse &other) {
  (void)other;
  return *this;
}
