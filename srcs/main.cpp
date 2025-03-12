/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:14 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/11 15:54:41 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Multiplexer.hpp"
#include "Server.hpp"
#include "config_parse.hpp"
#include "webserv.hpp"
#include <cstdlib>

int main(int argc, char **argv) {
  if (argc != 2)
    return (print_error_message("need conf filename"));
  try {
    Multiplexer &multiplexer = Multiplexer::get_instance();
    Parse parser(argv[1]);
    std::vector<std::map<std::string, std::vector<std::string> > >
        server_configs = parser.parse_nginx_config();
    if (server_configs.empty())
      throw std::runtime_error("No valid server configurations found.");
    Multiplexer::get_instance();
    std::vector<Server *> servers;
    for (size_t i = 0; i < server_configs.size(); i++)
      servers.push_back(new Server(server_configs[i]));
    for (size_t i = 0; i < servers.size(); i++)
      servers[i]->createSockets();
    multiplexer.run();
    for (size_t i = 0; i < servers.size(); i++)
      delete servers[i];
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
