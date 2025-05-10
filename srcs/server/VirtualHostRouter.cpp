/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   VirtualHostRouter.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 10:38:37 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/05/10 17:46:16 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "VirtualHostRouter.hpp"
#include "Server.hpp"
#include <cstddef>
#include <string>
#include <iostream>
VirtualHostRouter::VirtualHostRouter() : servers() {}

VirtualHostRouter::~VirtualHostRouter() {
  for (size_t i = 0; i < servers.size(); ++i) {
    delete servers[i];
  }
}

void VirtualHostRouter::add(Server *s){ 
  std::cout << "[DEBUG] Adding server: " 
            << (s->get_config().find("server_name") != s->get_config().end() 
                  ? s->get_config().find("server_name")->second[0] : "(none)")
            << " | is_default_server: " << s->is_default_server() << std::endl;
  if (s->is_default_server()) {
    for (size_t i = 0; i < servers.size(); ++i) {
      if (servers[i]->is_default_server()) {
        throw std::runtime_error("Duplicate default_server for this listen");
      }
    }
    servers.insert(servers.begin(), s);
  } else {
    servers.push_back(s);
  }
}

Server *VirtualHostRouter::route_by_host(const std::string &host) const {
  size_t colon_pos = host.find(':');
  std::string host_name;
  if (colon_pos != std::string::npos) {
    host_name = host.substr(0, colon_pos);
  } else {
    host_name = host;
  }
  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i]->matches_host(host_name)) {
      return servers[i];
    }
  }
  return servers.empty() ? NULL : servers[0];
}

VirtualHostRouter &
VirtualHostRouter::operator=(const VirtualHostRouter &other) {
  (void)other;
  return *this;
}
