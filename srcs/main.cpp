/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:14 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/22 13:18:42 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParse.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "types.hpp"

int main(int argc, char **argv) {
  if (argc != 2)
    return (print_error_message("need conf filename"));
  try {
    Parse parser(argv[1]);
    ServerAndLocationConfigs server_location_configs;
    server_location_configs = parser.parse_nginx_config();
    if (server_location_configs.empty())
      throw std::runtime_error("No valid server configurations found.");

    Multiplexer &multiplexer = Multiplexer::get_instance();
    std::vector<Server *> servers;
    for (size_t i = 0; i < server_location_configs.size(); i++)
      servers.push_back(new Server(server_location_configs[i].first,
                                   server_location_configs[i].second));
    for (size_t i = 0; i < servers.size(); i++)
      servers[i]->createSockets();
    multiplexer.run();
    multiplexer.delete_instance(); // fd close & clientのdelete
    for (size_t i = 0; i < servers.size(); i++)
      delete servers[i];
    debug_log.close();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    debug_log.close();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
