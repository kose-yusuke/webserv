/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/06/09 22:42:12 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

HttpResponse::HttpResponse() {}

HttpResponse::~HttpResponse() {}

ResponseEntry *HttpResponse::get_front_response() {
  LOG_DEBUG_FUNC();
  return response_deque_.empty() ? NULL : &response_deque_.front();
}

void HttpResponse::pop_front_response() {
  LOG_DEBUG_FUNC();
  response_deque_.pop_front();
}

bool HttpResponse::has_next_response() const {
  return (!response_deque_.empty());
}

void HttpResponse::push_front_response(ConnectionPolicy conn,
                                       const std::string &response,
                                       ResponseType type) {
  LOG_DEBUG_FUNC();
  if (response.empty()) {
    return;
  }
  struct ResponseEntry entry;
  entry.conn = conn;
  entry.buffer = response;
  entry.offset = 0;
  entry.type = type;
  response_deque_.push_front(entry);
}

void HttpResponse::push_back_response(ConnectionPolicy conn,
                                      const std::string &response,
                                      ResponseType type) {
  LOG_DEBUG_FUNC();
  if (response.empty()) {
    return;
  }
  struct ResponseEntry entry;
  entry.conn = conn;
  entry.buffer = response;
  entry.offset = 0;
  entry.type = type;
  response_deque_.push_back(entry);
}

void HttpResponse::generate_response(int status_code,
                                     const std::string &content,
                                     const std::string &content_type,
                                     ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " OK\r\n";
  response << "Content-Length: " << content.size() << "\r\n";
  response << "Content-Type: " << content_type << "\r\n";
  response << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
  response << content;

  push_back_response(conn, response.str(), NORMAL);
}

void HttpResponse::generate_chunk_response_header(ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << "HTTP/1.1 200 OK\r\n";
  response << "Content-Type: text/html; charset=UTF-8\r\n";
  response << "Transfer-Encoding: chunked\r\n";
  response << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
  push_back_response(conn, response.str(), CHUNK);
}

void HttpResponse::generate_chunk_response_body(const std::string &content,
                                                ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << std::hex << content.size() << "\r\n";
  response << content << "\r\n";
  push_back_response(conn, response.str(), CHUNK);
}

void HttpResponse::generate_chunk_response_last(ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << "0\r\n\r\n";
  push_back_response(conn, response.str(), CHUNK_LAST);
}

void HttpResponse::generate_custom_error_page(int status_code,
                                              const std::string &error_page,
                                              std::string _root,
                                              ConnectionPolicy conn) {
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
    response << "Content-Type: text/html\r\n";
    response << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
    response << file_content;

    push_back_response(conn, response.str(), NORMAL);
  } catch (const std::exception &e) {
    std::ostringstream fallback;
    fallback << "HTTP/1.1 " << status_code << " ";
    if (status_code == 404) {
      fallback << "Not Found";
    } else if (status_code == 405) {
      fallback << "Method Not Allowed";
    }
    fallback << "\r\n";
    fallback << "Content-Length: 9\r\n";
    fallback << "Content-Type: text/plain\r\n";
    fallback << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
    fallback << "Not Found";

    push_back_response(conn, fallback.str(), NORMAL);
  }
}

void HttpResponse::generate_error_response(int status_code,
                                           const std::string &message,
                                           ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
  response << "Content-Length: " << message.size() << "\r\n";
  response << "Content-Type: text/plain\r\n";
  response << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
  response << message;

  push_back_response(conn, response.str(), NORMAL);
}

void HttpResponse::generate_error_response(int status_code,
                                           ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();
  std::ostringstream response;
  const std::string &message = get_status_message(status_code);
  response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
  response << "Content-Length: " << message.size() << "\r\n";
  response << "Content-Type: text/plain\r\n";
  response << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
  response << message;

  push_back_response(conn, response.str(), NORMAL);
}

void HttpResponse::generate_redirect(int status_code,
                                     const std::string &new_location,
                                     ConnectionPolicy conn) {

  LOG_DEBUG_FUNC();
  std::ostringstream response;
  response << "HTTP/1.1 " << status_code << " ";
  response << HttpResponse::get_status_message(status_code) << "\r\n";
  response << "Location: " << new_location << "\r\n";
  response << "Content-Length: 0\r\n";
  response << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
  push_back_response(conn, response.str(), NORMAL);
}

void HttpResponse::generate_timeout_response() {
  LOG_DEBUG_FUNC();

  std::ostringstream content;
  content << "<html>\n"
          << "  <head>\n"
          << "    <title>408 Request Timeout</title>\n"
          << "  </head>\n"
          << "  <body>\n"
          << "    <h1>408 Request Timeout</h1>\n"
          << "    <p>Failed to process request in time. Please try again.</p>\n"
          << "  </body>\n"
          << "</html>";

  std::string content_str = content.str();

  std::ostringstream response;
  response << "HTTP/1.1 408 Request Timeout\r\n";
  response << "Content-Type: text/html\r\n";
  response << "Content-Length: " << content_str.size() << "\r\n";
  response << "Connection: close\r\n\r\n";
  response << content_str;

  push_front_response(CP_MUST_CLOSE, response.str(), NORMAL);
}

const char *HttpResponse::get_status_message(int status_code) {
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
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 408:
    return "Request Timeout";
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

const char *HttpResponse::to_connection_value(ConnectionPolicy conn) const {
  return conn == CP_KEEP_ALIVE ? "keep-alive" : "close";
}

HttpResponse &HttpResponse::operator=(const HttpResponse &other) {
  (void)other;
  return *this;
}
