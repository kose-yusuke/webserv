/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:51 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/06/09 02:46:44 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ResponseTypes.hpp"
#include <cstddef>
#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>

struct ResponseEntry {
  ConnectionPolicy conn;  // 送信後の接続処理
  std::string buffer;     // response
  size_t offset;          // 送信済みバイト数
  bool chunk_in_progress; // chunkedレスポンスの途中
};

class HttpResponse {
public:
  HttpResponse();
  ~HttpResponse();

  ResponseEntry *get_front_response();
  void pop_front_response();
  bool has_next_response() const;

  void generate_response(int status_code, const std::string &content,
                         const std::string &content_type,
                         ConnectionPolicy connection_policy);

  void generate_chunk_response_header(ConnectionPolicy connection_policy);

  void generate_chunk_response_body(const std::string &content,
                                    ConnectionPolicy conn);

  void generate_chunk_response_last(ConnectionPolicy conn);

  void generate_custom_error_page(int status_code,
                                  const std::string &error_page,
                                  std::string _root,
                                  ConnectionPolicy connection_policy);
  void generate_error_response(int status_code, const std::string &message,
                               ConnectionPolicy connection_policy);
  void generate_error_response(int status_code,
                               ConnectionPolicy connection_policy);

  void generate_redirect(int status_code, const std::string &new_location,
                         ConnectionPolicy conn);
  void generate_timeout_response();

private:
  std::deque<ResponseEntry> response_deque_;

  void push_front_response(ConnectionPolicy connection_policy,
                           const std::string &response, bool chunk_in_progress);
  void push_back_response(ConnectionPolicy connection_policy,
                          const std::string &response, bool chunk_in_progress);
  const char *get_status_message(int status_code);
  const char *to_connection_value(ConnectionPolicy conn) const;

  HttpResponse(const HttpResponse &other);
  HttpResponse &operator=(const HttpResponse &other);
};
