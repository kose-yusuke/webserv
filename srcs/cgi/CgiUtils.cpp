#include "CgiUtils.hpp"
#include "Utils.hpp"

namespace CgiUtils {
bool is_cgi_request(
    const std::string &path, const std::vector<std::string> &cgi_extensions) {
  std::string::size_type dot_pos = path.find_last_of('.');
  if (dot_pos == std::string::npos)
    return false;

  std::string extension = path.substr(dot_pos);
  for (size_t i = 0; i < cgi_extensions.size(); ++i) {
    if (cgi_extensions[i] == extension)
      return true;
  }
  return false;
}

bool is_location_has_cgi(ConfigMap best_match_config) {
  ConstConfigIt it = best_match_config.find("cgi_extensions");
  if (it == best_match_config.end() || it->second.empty())
    return false;
  return true;
}

bool is_cgi_like_path(const std::string& path) {
    static const char* default_exts[] = { ".cgi", ".pl", ".py", ".php", ".rb" };
    std::string ext = getExtension(path);

    for (size_t i = 0; i < sizeof(default_exts)/sizeof(*default_exts); ++i)
        if (ext == default_exts[i])
            return true;
    return false;
}

} // namespace CgiUtils
