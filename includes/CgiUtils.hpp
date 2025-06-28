#pragma once

#include "types.hpp"
#include <string>
#include <vector>

namespace CgiUtils {

bool is_cgi_request(const std::string &path,
                    const std::vector<std::string> &cgi_extensions);

bool is_location_has_cgi(ConfigMap best_match_config);
bool is_cgi_like_path(const std::string& path);

} // namespace CgiUtils
