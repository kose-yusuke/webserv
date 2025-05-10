/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   VirtualHostRouter.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 10:38:37 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/05/10 18:28:47 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "VirtualHostRouter.hpp"
#include "Server.hpp"
#include "ConfigParse.hpp"

VirtualHostRouter::VirtualHostRouter() : servers() {}

VirtualHostRouter::~VirtualHostRouter() {
  for (size_t i = 0; i < servers.size(); ++i) {
    delete servers[i];
  }
}

void VirtualHostRouter::add(Server *s){ 
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

// Server *VirtualHostRouter::route_by_host(const std::string &host) const {
//   size_t colon_pos = host.find(':');
//   std::string host_name;
//   if (colon_pos != std::string::npos) {
//     host_name = host.substr(0, colon_pos);
//   } else {
//     host_name = host;
//   }
//   for (size_t i = 0; i < servers.size(); ++i) {
//     if (servers[i]->matches_host(host_name)) {
//       return servers[i];
//     }
//   }
//   return servers.empty() ? NULL : servers[0];
// }
Server *VirtualHostRouter::route_by_host(const std::string &host) const {
  Server* best_match = NULL;
  size_t best_length = 0;

  size_t colon_pos = host.find(':');
  std::string host_name;
  if (colon_pos != std::string::npos) {
    host_name = host.substr(0, colon_pos);
  } else {
    host_name = host;
  }

  for (size_t i = 0; i < servers.size(); ++i) {
      const std::vector<std::string>& names = servers[i]->get_config().find("server_name")->second;

      for (size_t j = 0; j < names.size(); ++j) {
          const std::string& pattern = names[j];

          // 完全一致（最優先）
          if (pattern == host_name) {
              return servers[i];
          }

          // ワイルドカード前方一致（*.example.com）
          if (pattern.length() > best_length && parser.wildcard_suffix_match(pattern, host_name)) {
              best_match = servers[i];
              best_length = pattern.length();
          }

          // ワイルドカード後方一致（www*）
          else if (pattern.length() > best_length && parser.wildcard_prefix_match(pattern, host_name)) {
              best_match = servers[i];
              best_length = pattern.length();
          }
      }
  }

  if (best_match) 
    return best_match;

  // 最後に default_server を返す
  for (size_t i = 0; i < servers.size(); ++i) {
      if (servers[i]->is_default_server()) {
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
