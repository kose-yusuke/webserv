/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:44:51 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/12 15:26:57 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <sys/socket.h>

class HttpRequest;
class HttpResponse {
public:
  static std::string generate(const HttpRequest &request, int server_fd);

  static void send_custom_error_page(int client_socket, int status_code,
                                     const std::string &error_page);
  static void send_error_response(int clientFd, int status_code,
                                  const std::string &message);
  static void send_response(int client_socket, int status_code,
                            const std::string &content,
                            const std::string &content_type);
};
