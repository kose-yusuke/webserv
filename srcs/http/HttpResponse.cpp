/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/04/16 17:26:41 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include "Utils.hpp"

HttpResponse::HttpResponse() {}

HttpResponse::~HttpResponse() {}

bool HttpResponse::has_next_response() const { return !response_queue.empty(); }

std::string HttpResponse::get_next_response() {
  LOG_DEBUG_FUNC();
  if (response_queue.empty()) {
    return "";
  }
  return response_queue.front();
}

void HttpResponse::pop_response() {
  LOG_DEBUG_FUNC();
  response_queue.pop();
}

void HttpResponse::push_response(const std::string &response) {
  LOG_DEBUG_FUNC();
  if (response.empty()) {
    return;
  }
  response_queue.push(response);
}

void HttpResponse::generate_response(int status_code,
                                     const std::string &content,
                                     const std::string &content_type) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " OK\r\n";
  response << "Content-Length: " << content.size() << "\r\n";
  response << "Content-Type: " << content_type << "\r\n\r\n";
  response << content;

  push_response(response.str());
  // send(client_socket, response.str().c_str(), response.str().size(), 0);
}

void HttpResponse::generate_custom_error_page(int status_code,
                                              const std::string &error_page, std::string _root) {
  LOG_DEBUG_FUNC();
  try {
    std::string file_content = read_file(_root + error_page);

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
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
  response << "Content-Length: " << message.size() << "\r\n";
  response << "Content-Type: text/plain\r\n\r\n";
  response << message;

  push_response(response.str());
  // send(client_socket, response.str().c_str(), response.str().size(), 0);
}

void HttpResponse::generate_error_response(int status_code) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  const std::string &message = get_status_message(status_code);
  response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
  response << "Content-Length: " << message.size() << "\r\n";
  response << "Content-Type: text/plain\r\n\r\n";
  response << message;

  push_response(response.str());
}

void HttpResponse::generate_redirect(int status_code,
                                     const std::string &new_location) {

  LOG_DEBUG_FUNC();
  std::ostringstream response;
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
  case 303:
    return "See Other";
  case 307:
    return "Temporary Redirect";
  case 308:
    return "Permanent Redirect";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 413:
    return "Payload Too Large";
  case 414:
    return "URI Too Long";
  case 431:
    return "Request Header Fields Too Large";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 505:
    return "HTTP Version Not Supported";
  default:
    logfd(LOG_ERROR, "Undefined status code detected: ", status_code);
    return "Status Not Defined";
  }
}

HttpResponse::HttpResponse(const HttpResponse &other) { (void)other; }

HttpResponse &HttpResponse::operator=(const HttpResponse &other) {
  (void)other;
  return *this;
}
