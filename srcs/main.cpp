/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:14 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/06/27 22:39:58 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiRegistry.hpp"
#include "ClientRegistry.hpp"
#include "ConfigParse.hpp"
#include "Logger.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "ServerBuilder.hpp"
#include "ServerRegistry.hpp"
#include "types.hpp"
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

static void free_resources() {
  Multiplexer::delete_instance();
  debug_log.close();
}

static void handle_sigchld(int sig) {
  const int saved_errno = errno;
  (void)sig;

  for (;;) {
    pid_t pid = waitpid(-1, NULL, WNOHANG);
    if (pid <= 0) {
      if (pid == -1 && (errno == EINTR || errno == EAGAIN)) {
        continue;
      }
      break;
    } else {
      std::cerr << "[SIGCHLD] Reaped child pid: " << pid << std::endl;
    }
  }

  errno = saved_errno;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return (print_error_message("need conf filename"));

  Multiplexer &multiplexer = Multiplexer::get_instance();
  std::atexit(free_resources);

  signal(SIGPIPE, SIG_IGN);        // client終了時のcrash予防; SIGPIPEを無視
  signal(SIGCHLD, handle_sigchld); // CGIの子process回収

  try {
    Parse parser(argv[1]);
    ServerAndLocationConfigs server_location_configs;
    server_location_configs = parser.parse_nginx_config();
    if (server_location_configs.empty())
      throw std::runtime_error("No valid server configurations found.");

    ServerRegistry server_registry;
    ClientRegistry client_registry;
    CgiRegistry cgi_registry;

    ServerBuilder::build(server_location_configs, server_registry);
    server_registry.initialize();

    multiplexer.set_server_registry(&server_registry);
    multiplexer.set_client_registry(&client_registry);
    multiplexer.set_cgi_registry(&cgi_registry);

    multiplexer.run();

    free_resources();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    free_resources();
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
