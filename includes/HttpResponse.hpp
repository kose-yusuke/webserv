/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:51 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/04/16 15:11:16 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <sys/socket.h>

class HttpResponse {
public:
  HttpResponse();
  ~HttpResponse();

  bool has_next_response() const;
  std::string get_next_response();
  void pop_response();

  void generate_custom_error_page(int status_code,
                                  const std::string &error_page, std::string _root);
  void generate_error_response(int status_code, const std::string &message);
  void generate_error_response(int status_code);
  void generate_response(int status_code, const std::string &content,
                         const std::string &content_type);
  void generate_redirect(int status_code, const std::string &new_location);

private:
  std::queue<std::string> response_queue;

  void push_response(const std::string &response);
  std::string get_status_message(int status_code);

  HttpResponse(const HttpResponse &other);
  HttpResponse &operator=(const HttpResponse &other);
};
