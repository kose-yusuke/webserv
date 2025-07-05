/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/05 01:42:15 by sakitaha          #+#    #+#             */
/*   Updated: 2025/07/05 02:35:26 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "MimeTypes.hpp"
#include <map>

namespace MimeTypes {

std::string get_mime_type(const std::string &path) {
  static std::map<std::string, std::string> mime_types;
  if (mime_types.empty()) {
    mime_types[".html"] = "text/html";
    mime_types[".css"] = "text/css";
    mime_types[".js"] = "application/javascript";
    mime_types[".png"] = "image/png";
    mime_types[".jpg"] = "image/jpeg";
    mime_types[".jpeg"] = "image/jpeg";
    mime_types[".gif"] = "image/gif";
    mime_types[".svg"] = "image/svg+xml";
    mime_types[".json"] = "application/json";
    mime_types[".txt"] = "text/plain";
    mime_types[".ico"] = "image/x-icon";
  }

  size_t dot_pos = path.find_last_of('.');
  if (dot_pos != std::string::npos) {
    std::string ext = path.substr(dot_pos);
    std::map<std::string, std::string>::const_iterator it =
        mime_types.find(ext);
    if (it != mime_types.end()) {
      return it->second;
    }
  }
  return "application/octet-stream";
}

} // namespace MimeTypes
