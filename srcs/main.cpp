/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:14 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/04/15 18:31:48 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParse.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "types.hpp"

static void free_resources(Multiplexer *multiplexer,
                           std::vector<Server *> &servers) {
  if (multiplexer) {
    multiplexer->delete_instance(); // fd close & clientのdelete
  }
  for (size_t i = 0; i < servers.size(); i++) {
    delete servers[i];
  }
  debug_log.close();
}

int main(int argc, char **argv) {
  if (argc != 2)
    return (print_error_message("need conf filename"));

  Multiplexer *multiplexer = NULL;
  std::vector<Server *> servers;

  try {
    Parse parser(argv[1]);
    ServerAndLocationConfigs server_location_configs;
    server_location_configs = parser.parse_nginx_config();
    if (server_location_configs.empty())
      throw std::runtime_error("No valid server configurations found.");

    multiplexer = &Multiplexer::get_instance();
    for (size_t i = 0; i < server_location_configs.size(); i++)
      servers.push_back(new Server(server_location_configs[i].first,
                                    server_location_configs[i].second));
    for (size_t i = 0; i < servers.size(); i++)
      servers[i]->createSockets();

    multiplexer->run();

    free_resources(multiplexer, servers);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    free_resources(multiplexer, servers);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
