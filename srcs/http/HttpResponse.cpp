/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/14 01:36:27 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "Utils.hpp"

void HttpResponse::send_response(int client_socket, int status_code, const std::string &content, const std::string &content_type) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " OK\r\n";
    response << "Content-Length: " << content.size() << "\r\n";
    response << "Content-Type: " << content_type << "\r\n\r\n";
    response << content;

    send(client_socket, response.str().c_str(), response.str().size(), 0);
}


void HttpResponse::send_custom_error_page(int client_socket, int status_code, const std::string &error_page) {
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

    send(client_socket, response.str().c_str(), response.str().size(), 0);
  } catch (const std::exception &e) {
    std::ostringstream fallback;
    fallback << "HTTP/1.1 " << status_code << " ";
    if (status_code == 404) {
      fallback << "Not Found";
    } else if (status_code == 405) {
      fallback << "Method Not Allowed";
    }
    fallback << "\r\nContent-Length: 9\r\n\r\nNot Found";
    send(client_socket, fallback.str().c_str(), fallback.str().size(), 0);
  }
}

void HttpResponse::send_error_response(int client_socket, int status_code,
                                       const std::string &message) {
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
  response << "Content-Length: " << message.size() << "\r\n";
  response << "Content-Type: text/plain\r\n\r\n";
  response << message;

  send(client_socket, response.str().c_str(), response.str().size(), 0);
}

// 未実装
void HttpResponse::send_redirect(int client_socket, int status_code, const std::string new_location)
{
    std::cout << client_socket << std::endl;
    std::cout << status_code << std::endl;
    std::cout << new_location << std::endl;
}
