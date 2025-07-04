/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:08 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/07/05 03:57:41 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

HttpResponse::HttpResponse() {}

HttpResponse::~HttpResponse() {}

ResponseEntry *HttpResponse::get_front_response() {
  LOG_DEBUG_FUNC();
  return response_queue_.empty() ? NULL : &response_queue_.front();
}

bool HttpResponse::has_response() const { return (!response_queue_.empty()); }

void HttpResponse::push_back_response(ConnectionPolicy conn,
                                      const std::vector<char> &buf) {
  LOG_DEBUG_FUNC();
  if (buf.empty()) {
    return;
  }
  struct ResponseEntry entry;
  entry.conn = conn;
  entry.buffer = buf;
  entry.offset = 0;
  response_queue_.push(entry);
}

void HttpResponse::push_back_response(ConnectionPolicy conn,
                                      std::ostringstream &oss) {
  std::string str = oss.str();
  if (str.empty()) {
    return;
  }
  struct ResponseEntry entry;
  entry.conn = conn;
  entry.buffer = std::vector<char>(str.begin(), str.end());
  entry.offset = 0;
  response_queue_.push(entry);
}

void HttpResponse::pop_front_response() {
  LOG_DEBUG_FUNC();
  response_queue_.pop();
}

void HttpResponse::generate_response(int status_code,
                                     const std::vector<char> &content,
                                     const std::string &content_type,
                                     ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();

  std::ostringstream oss;
  oss << "HTTP/1.1 " << status_code << " OK\r\n";
  if (status_code != 204) {
    oss << "Content-Length: " << content.size() << "\r\n";
    oss << "Content-Type: " << content_type << "\r\n";
  }
  oss << "Connection: " << to_connection_value(conn) << "\r\n\r\n";
  std::string header = oss.str();

  std::vector<char> response;
  response.insert(response.end(), header.begin(), header.end());
  response.insert(response.end(), content.begin(), content.end());

  push_back_response(conn, response);
}

void HttpResponse::generate_response(
    int status_code,
    const std::vector<std::pair<std::string, std::string> > &headers,
    const std::vector<char> &body, ConnectionPolicy conn) {
  LOG_DEBUG_FUNC();

  std::ostringstream oss;
  oss << "HTTP/1.1 " << status_code << " " << get_status_message(status_code)
      << "\r\n";
  for (size_t i = 0; i < headers.size(); ++i) {
    oss << headers[i].first << ": " << headers[i].second << "\r\n";
  }
  oss << "Connection: " << to_connection_value(conn) << "\r\n\r\n";

  std::string header_str = oss.str();
  std::vector<char> full_response(header_str.begin(), header_str.end());
  full_response.insert(full_response.end(), body.begin(), body.end());

  push_back_response(conn, full_response);
}

void HttpResponse::generate_chunk_response_header(
    int status_code,
    const std::vector<std::pair<std::string, std::string> > &headers,
    ConnectionPolicy conn_policy) {
  LOG_DEBUG_FUNC();

  std::ostringstream oss;
  oss << "HTTP/1.1 " << status_code << " " << get_status_message(status_code)
      << "\r\n";
  for (size_t i = 0; i < headers.size(); ++i) {
    oss << headers[i].first << ": " << headers[i].second << "\r\n";
  }
  oss << "Connection: " << to_connection_value(conn_policy) << "\r\n\r\n";

  std::string header_str = oss.str();
  std::vector<char> header_vec(header_str.begin(), header_str.end());
  // NOTE: header 送信時点では、接続の終了判断はしない（必ず keep-alive にする）
  push_back_response(CP_KEEP_ALIVE, header_vec);
}

void HttpResponse::generate_chunk_response_body(const std::vector<char> &data) {
  LOG_DEBUG_FUNC();
  if (data.empty()) {
    return;
  }
  static const char kCRLF[] = "\r\n";
  std::ostringstream oss;
  std::vector<char> chunk;

  oss << std::hex << data.size() << kCRLF;
  std::string size_line = oss.str();

  chunk.insert(chunk.end(), size_line.begin(), size_line.end());
  chunk.insert(chunk.end(), data.begin(), data.end());
  chunk.insert(chunk.end(), kCRLF, kCRLF + 2);
  // NOTE: last chunk 未送信時点では、接続の終了判断はしない（必ず keep-alive）
  push_back_response(CP_KEEP_ALIVE, chunk);
}

void HttpResponse::generate_chunk_response_last(ConnectionPolicy conn_policy) {
  LOG_DEBUG_FUNC();
  static const char k_chunk_end_marker[] = "0\r\n\r\n";
  std::vector<char> chunk;

  chunk.insert(chunk.end(), k_chunk_end_marker, k_chunk_end_marker + 5);
  push_back_response(conn_policy, chunk);
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

    push_back_response(conn, response);

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

    push_back_response(conn, fallback);
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

  push_back_response(conn, response);
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

  push_back_response(conn, response);
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
  push_back_response(conn, response);
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

  push_back_response(CP_MUST_CLOSE, response);
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
