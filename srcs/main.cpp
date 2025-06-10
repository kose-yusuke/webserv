/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:14 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/06/11 02:33:52 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientRegistry.hpp"
#include "ConfigParse.hpp"
#include "Logger.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "ServerBuilder.hpp"
#include "ServerRegistry.hpp"
#include "ZombieRegistry.hpp"
#include "types.hpp"
#include <signal.h>

static void free_resources() {
  Multiplexer::delete_instance();
  debug_log.close();
}

int main(int argc, char **argv) {
  if (argc != 2)
    return (print_error_message("need conf filename"));

  Multiplexer &multiplexer = Multiplexer::get_instance();
  signal(SIGPIPE, SIG_IGN);

  try {
    Parse parser(argv[1]);
    ServerAndLocationConfigs server_location_configs;
    server_location_configs = parser.parse_nginx_config();
    if (server_location_configs.empty())
      throw std::runtime_error("No valid server configurations found.");

    ServerRegistry server_registry;
    ClientRegistry client_registry;
    ZombieRegistry zombie_registry;

    ServerBuilder::build(server_location_configs, server_registry);
    server_registry.initialize();

    multiplexer.set_server_registry(&server_registry);
    multiplexer.set_client_registry(&client_registry);
    multiplexer.set_zombie_registry(&zombie_registry);

    multiplexer.run();

    free_resources();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    free_resources();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
