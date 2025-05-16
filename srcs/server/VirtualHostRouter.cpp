/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   VirtualHostRouter.cpp                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 10:38:37 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/05/16 21:02:52 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "VirtualHostRouter.hpp"
#include "Server.hpp"
#include "ConfigParse.hpp"

VirtualHostRouter::VirtualHostRouter() : servers_() {}

VirtualHostRouter::~VirtualHostRouter() {
  for (size_t i = 0; i < servers_.size(); ++i) {
    delete servers_[i];
  }
}

void VirtualHostRouter::add(Server *s){
  if (s->is_default_server()) {
    for (size_t i = 0; i < servers_.size(); ++i) {
      if (servers_[i]->is_default_server()) {
        throw std::runtime_error("Duplicate default_server for this listen");
      }
    }
    servers_.insert(servers_.begin(), s);
  } else {
    servers_.push_back(s);
  }
}

Server *VirtualHostRouter::route_by_host(const std::string &host) const {
  Server* best_match = NULL;
  size_t best_length = 0;
  size_t colon_pos = host.find(':');
  std::string host_name;

  if (colon_pos != std::string::npos)
    host_name = host.substr(0, colon_pos);
  else
    host_name = host;

  for (size_t i = 0; i < servers_.size(); ++i) {
      const std::vector<std::string>& names = servers_[i]->get_server_names();

      for (size_t j = 0; j < names.size(); ++j) {
          const std::string& pattern = names[j];

          // 完全一致（最優先）
          if (pattern == host_name)
              return servers_[i];

          // ワイルドカード前方一致（*.example.com）
          if (pattern.length() > best_length && parser_.wildcard_suffix_match(pattern, host_name)) {
              best_match = servers_[i];
              best_length = pattern.length();
          }

          // ワイルドカード後方一致（www*）
          else if (pattern.length() > best_length && parser_.wildcard_prefix_match(pattern, host_name)) {
              best_match = servers_[i];
              best_length = pattern.length();
          }
      }
  }

  if (best_match)
    return best_match;

  // 最後に default_server を返す
  for (size_t i = 0; i < servers_.size(); ++i) {
      if (servers_[i]->is_default_server()) {
          return servers_[i];
      }
  }

  return servers_.empty() ? NULL : servers_[0];
}

VirtualHostRouter &
VirtualHostRouter::operator=(const VirtualHostRouter &other) {
  (void)other;
  return *this;
}
