/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:51 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/07/05 01:26:16 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "ResponseTypes.hpp"
#include <cstddef>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <sys/socket.h>

// HttpResponse はレスポンスキュー管理の責務

struct ResponseEntry {
  ConnectionPolicy conn;    // 送信後の接続処理
  std::vector<char> buffer; // レスポンス本体（header＋body含む）
  size_t offset;            // 送信済みバイト数
};

class HttpResponse {
public:
  HttpResponse();
  ~HttpResponse();

  ResponseEntry *get_front_response();
  bool has_response() const;
  void push_back_response(ConnectionPolicy conn, const std::vector<char> &buf);
  void push_back_response(ConnectionPolicy conn, std::ostringstream &oss);
  void pop_front_response();

  void generate_response(int status_code, const std::vector<char> &content,
                         const std::string &content_type,
                         ConnectionPolicy connection_policy);

  void generate_response(
      int status_code,
      const std::vector<std::pair<std::string, std::string> > &headers,
      const std::vector<char> &body, ConnectionPolicy connnection_policy);

  void generate_chunk_response_header(
      int status_code,
      const std::vector<std::pair<std::string, std::string> > &headers,
      ConnectionPolicy connnection_policy);

  void generate_chunk_response_body(const std::vector<char> &body);

  void generate_chunk_response_last(ConnectionPolicy connnection_policy);

  void generate_custom_error_page(int status_code,
                                  const std::string &error_page,
                                  std::string _root,
                                  ConnectionPolicy connection_policy);

  void generate_error_response(int status_code, const std::string &message,
                               ConnectionPolicy connection_policy);

  void generate_error_response(int status_code,
                               ConnectionPolicy connection_policy);

  void generate_redirect(int status_code, const std::string &new_location,
                         ConnectionPolicy connection_policy);

  void generate_timeout_response();

private:
  std::queue<ResponseEntry> response_queue_;

  const char *get_status_message(int status_code);
  const char *to_connection_value(ConnectionPolicy conn) const;

  HttpResponse(const HttpResponse &other);
  HttpResponse &operator=(const HttpResponse &other);
};
