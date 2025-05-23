/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:05 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/05/24 13:06:00 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "Utils.hpp"
#include "VirtualHostRouter.hpp"
#include "CgiHandler.hpp"

const size_t HttpRequest::k_default_max_body = 104857600;

HttpRequest::HttpRequest(const VirtualHostRouter *router,
                         HttpResponse &httpResponse)
    : is_autoindex_enabled(false), response(httpResponse),
      virtual_host_router(router), cgi(NULL), connection_policy(CP_KEEP_ALIVE),
      status_code(0), max_body_size(k_default_max_body) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::select_server_by_host() {
  LOG_DEBUG_FUNC();

  const std::string host_name = get_header_value("Host");
  Server *server = virtual_host_router->route_by_host(host_name);
  this->server_config = server->get_config();
  this->location_configs = server->get_locations();
}

void HttpRequest::init_cgi_extensions() {
  ConstConfigIt it = best_match_config.find("cgi_extensions");
  if (it != best_match_config.end() && !it->second.empty()) {
    cgi_extensions = it->second;
  }
}

void HttpRequest::init_autoindex() {
  ConstConfigIt it = best_match_config.find("autoindex");
  if (it != best_match_config.end() && !it->second.empty()) {
    is_autoindex_enabled = (it->second[0] == "on");
  } else {
    is_autoindex_enabled = false;
  }
}

void HttpRequest::init_file_index() {
  ConstConfigIt index_it = best_match_config.find("index");
  if (index_it != best_match_config.end() && !index_it->second.empty()) {
    index_file_name = index_it->second[0];
  } else {
    index_file_name = "index.html";
  }
}

void HttpRequest::conf_init() {
  LOG_DEBUG_FUNC();
  this->best_match_config = get_best_match_config(path);

  if (!best_match_config["root"].empty())
    _root = best_match_config["root"][0];
  else if (!server_config["root"].empty())
    _root = server_config["root"][0];
  else
    print_error_message("No root found in config file.");
  set_cgi_handler(cgi);
  init_cgi_extensions();
  init_autoindex();
  init_file_index();
  if (best_match_config.count("error_page")) {
    error_page_map = extract_error_page_map(best_match_config["error_page"]);
  }
}

std::map<int, std::string>
HttpRequest::extract_error_page_map(const std::vector<std::string> &tokens) {
  std::map<int, std::string> result;
  size_t i = 0;

  while (i < tokens.size()) {
    std::vector<int> codes;

    while (i < tokens.size() && is_all_digits(tokens[i])) {
      codes.push_back(std::atoi(tokens[i].c_str()));
      ++i;
    }

    if (i >= tokens.size()) {
      throw std::runtime_error(
          "error_page parse error: missing path after status codes");
    }

    std::string path = tokens[i++];
    for (size_t j = 0; j < codes.size(); ++j) {
      result[codes[j]] = path;
    }
  }

  return result;
}

void HttpRequest::handle_http_request() {
  LOG_DEBUG_FUNC();
  select_server_by_host();
  conf_init();
  print_best_match_config(best_match_config);

  if (status_code != 0) {
    // TODO: kosekiさんが実装済みの、custom error pageの呼び出しを反映させる
    response.generate_error_response(status_code, connection_policy);
    return;
  }

  if (handle_redirection() != REDIR_NONE) {
    return;
  }

  ConstConfigIt method_it = best_match_config.find("allow_methods");
  if (method_it != best_match_config.end()) {
    allow_methods = method_it->second;
  } else {
    allow_methods.push_back("GET");
    allow_methods.push_back("POST");
    allow_methods.push_back("DELETE");
  }

  std::vector<std::string>::const_iterator it =
      std::find(allow_methods.begin(), allow_methods.end(), method);
  if (it != allow_methods.end()) {
    if (method == "GET") {
      handle_get_request(path);
    } else if (method == "POST") {
      handle_post_request();
    } else if (method == "DELETE") {
      handle_delete_request(path);
    }
  } else {
    response.generate_error_response(405, "Method Not Allowed",
                                     connection_policy);
  }
}

bool HttpRequest::parse_http_request(const std::string &request,
                                     std::string &method, std::string &path,
                                     std::string &version) {
  std::istringstream request_stream(request);
  if (!(request_stream >> method >> path >> version)) {
    return false;
  }
  return true;
}

void HttpRequest::merge_config(ConfigMap &base, const ConfigMap &override) {
  for (ConstConfigIt it = override.begin(); it != override.end(); ++it) {
    base[it->first] = it->second;
  }
}

ConfigMap HttpRequest::get_best_match_config(const std::string &path) {

  ConfigMap best_config;

  // まず, best_configにserverconfigのdirectiveを代入
  best_config = server_config;

  // 最もマッチする `location` を探す旅にでます
  std::string best_match = "/";

  // 1. 完全一致(= /path) を評価
  for (ConstLocationIt it = location_configs.begin();
       it != location_configs.end(); ++it) {
    const std::string &loc = it->first;
    if (loc.substr(0, 2) == "= ") {
      if (loc.substr(2) == path) {
        merge_config(best_config, it->second);
        return best_config;
      }
    }
  }

  // 2. 最長前方一致を記録（^~ の有無も記録）
  std::string longest_prefix = "/";
  bool has_caret_tilde = false;
  ConstLocationIt longest_prefix_it = location_configs.find("/");

  for (ConstLocationIt it = location_configs.begin();
       it != location_configs.end(); ++it) {
    const std::string &loc = it->first;
    if (loc.substr(0, 3) == "^~ ") {
      std::string clean_loc = loc.substr(3);
      if (path.find(clean_loc) == 0 &&
          clean_loc.length() > longest_prefix.length()) {
        longest_prefix = clean_loc;
        longest_prefix_it = it;
        has_caret_tilde = true;
      }
    } else if (loc[0] != '=' && loc[0] != '~' && path.find(loc) == 0 &&
               loc.length() > longest_prefix.length()) {
      longest_prefix = loc;
      longest_prefix_it = it;
    }
  }

  if (has_caret_tilde) {
    merge_config(best_config, longest_prefix_it->second);
    return best_config;
  }

  // 3. 正規表現マッチを探す
  for (ConstLocationIt it = location_configs.begin();
       it != location_configs.end(); ++it) {
    const std::string &loc = it->first;
    if (loc.substr(0, 2) == "~ " || loc.substr(0, 3) == "~* ") {
      std::string pattern = loc.substr(loc[1] == '*' ? 3 : 2);
      bool ignore_case = (loc[1] == '*');
      if (regex_match_posix(path, pattern, ignore_case)) {
        merge_config(best_config, it->second);
        return best_config;
      }
    }
  }

  // 4. 正規表現マッチがなければ、記録した最長prefixマッチ（^~なし）を使う
  if (longest_prefix_it != location_configs.end()) {
    merge_config(best_config, longest_prefix_it->second);
  }

  return (best_config);
}

void HttpRequest::load_max_body_size() {
  ConstConfigIt it = server_config.find("client_max_body_size");
  if (it != server_config.end()) {
    std::string max_size_str = it->second.front();
    max_body_size = str_to_size(max_size_str);
  }
  logfd(LOG_DEBUG, "client_max_body_size loaded: ", max_body_size);
}

/*GET Request*/
void HttpRequest::handle_get_request(std::string path) {

  std::string file_path = get_requested_resource(path);
  if (file_path.empty()) {
    handle_error(404);
    return;
  }

  ResourceType type = get_resource_type(file_path);

  if (type == Directory) {
    handle_directory_request(path);
  } else if (type == File) {
    if (cgi && cgi->is_cgi_request(file_path, cgi_extensions))
      cgi->handle_cgi_request(file_path, body_data, path, method);
    else
      handle_file_request(file_path);
  } else {
    handle_error(404);
  }

  if (type == Directory) {
    handle_directory_request(path);
  } else if (type == File) {
    if (cgi && cgi->is_location_has_cgi(best_match_config) && cgi->is_cgi_request(path, cgi_extensions))
      cgi->handle_cgi_request(file_path, body_data, path, method);
    else
      handle_file_request(file_path);
  } else {
    handle_error(404);
  }
}

std::string HttpRequest::get_requested_resource(const std::string &path) {
  std::string file_path = _root + path;

  if (!file_exists(file_path)) {
    return "";
  }
  return file_path;
}

ResourceType HttpRequest::get_resource_type(const std::string &path) {
  if (is_directory(path)) {
    return Directory;
  } else if (file_exists(path)) {
    return File;
  }
  return NotFound;
}

/*Requestがディレクトリかファイルかの分岐処理*/
void HttpRequest::handle_file_request(const std::string &file_path) {
  LOG_DEBUG_FUNC();

  std::ifstream file(file_path.c_str(), std::ios::in);
  if (!file.is_open()) {
    handle_error(404);
    return;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  std::string file_content = buffer.str();
  response.generate_response(200, file_content, "text/html", connection_policy);
}

void HttpRequest::handle_directory_request(std::string path) {
  // URLの末尾に `/` がない場合、リダイレクト（301）
  if (!ends_with(path, "/")) {
    std::string new_location = path + "/";
    response.generate_redirect(301, new_location, connection_policy);
    return;
  }

  // `index.html` が存在するか確認 - 本当はこの辺の　public
  // になっているところはrootとかで置き換える必要あり
  if (has_index_file(_root + path, index_file_name)) {
    handle_file_request(_root + path + index_file_name);
  } else {
    // autoindexがONの場合、ディレクトリリストを生成する
    if (is_autoindex_enabled) {
      std::string dir_listing = generate_directory_listing(_root + path);
      response.generate_response(200, dir_listing, "text/html",
                                 connection_policy);
    } else {
      handle_error(403);
    }
  }
}

bool HttpRequest::is_location_upload_file(const std::string file_path) {
  // 親ディレクトリ取得
  size_t last_slash = file_path.find_last_of('/');
  if (last_slash == std::string::npos) {
    std::cerr << "Invalid file path: " << file_path << std::endl;

    // HttpResponse::send_error_response(client_socket, 400, "Bad Request");
    response.generate_error_response(400, "Bad Request", connection_policy);
    return false;
  }
  std::string parent_dir = file_path.substr(0, last_slash);
  if (!is_directory(parent_dir)) {
    std::cerr << "Parent directory does not exist: " << parent_dir << std::endl;
    // HttpResponse::send_error_response(client_socket, 404, "Parent Directory
    // Not Found");
    response.generate_error_response(404, "Parent Directory Not Found",
                                     connection_policy);
    return false;
  }
  // 書き込み権限
  if (access(parent_dir.c_str(), W_OK) != 0) {
    response.generate_error_response(403, "Forbidden", connection_policy);
    return false;
  }

  if (file_exists(file_path)) {
    if (access(file_path.c_str(), W_OK) != 0) {
      response.generate_error_response(403, "Forbidden", connection_policy);
      return false;
    }
  }

  return true;
}

void HttpRequest::handle_post_request() {
  std::string full_path = _root + path;

  if (cgi && cgi->is_location_has_cgi(best_match_config) && cgi->is_cgi_request(path, cgi_extensions)) {
    cgi->handle_cgi_request(full_path, body_data, path, method);
    return;
  }

  if (!is_location_upload_file(full_path)) {
    handle_get_request(path); // POSTが許されない場所ならGETにフォールバック
    return;
  }

  std::string body(body_data.begin(), body_data.end());
  std::cout << "Received POST body: " << body << std::endl;

  if (body.empty()) {
    response.generate_response(204, "", "text/plain", connection_policy);
    return;
  }

  std::string upload_path = _root + path;
  std::ofstream ofs(upload_path.c_str());
  if (!ofs) {
    std::cerr << "Failed to open file: " << upload_path << std::endl;
    response.generate_error_response(
        500, "Internal Server Error: Failed to open file", connection_policy);
    return;
  }

  ofs << body;
  ofs.close();

  std::cout << "File written successfully: " << path << std::endl;
  response.generate_response(201, body, "text/plain", connection_policy);
}

void HttpRequest::handle_delete_request(const std::string path) {
  std::string file_path = get_requested_resource(path);
  std::cout << file_path << std::endl;

  if (file_path.empty()) {
    handle_error(404);
    return;
  }

  ResourceType type = get_resource_type(file_path);

  // 書き込み権限
  if (access(file_path.c_str(), W_OK) != 0) {
    response.generate_error_response(403, "Forbidden", connection_policy);
    return;
  }

  int status = -1;
  if (type == Directory) {
    status = handle_directory_delete(file_path);
  } else if (type == File) {
    if (cgi && cgi->is_cgi_request(file_path, cgi_extensions))
      cgi->handle_cgi_request(file_path, body_data, path, method);
    else {
      status = handle_file_delete(file_path);
      if (status == -1)
        handle_error(404);
    }
  } else {
    handle_error(404);
  }

  if (status == 0) {
    response.generate_response(204, "", "text/plain", connection_policy);
  } else {
    response.generate_error_response(500, "Internal Server Error",
                                     connection_policy);
  }
}

int HttpRequest::handle_file_delete(const std::string &file_path) {
  if (std::remove(file_path.c_str()) == 0) {
    std::cout << "File deleted successfully: " << file_path << std::endl;
    return 0;
  } else {
    std::cerr << "Failed to delete file: " << file_path << std::endl;
    return -1;
  }
  return 0;
}

int HttpRequest::handle_directory_delete(const std::string &dir_path) {
  if (!ends_with(dir_path, "/")) {
    response.generate_error_response(409, "Conflict", connection_policy);
    return -1;
  }

  if (has_index_file(dir_path, index_file_name)) {
    response.generate_error_response(403, "Forbidden", connection_policy);
    return -1;
  }

  if (delete_all_directory_content(dir_path) != 0) {
    response.generate_error_response(500, "Internal Server Error",
                                     connection_policy);
    return -1;
  }

  std::string html = dir_path;
  return 0;
}

int HttpRequest::delete_all_directory_content(const std::string &dir_path) {
  DIR *dir = opendir(dir_path.c_str());
  if (!dir)
    return -1;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == "." || name == "..")
      continue;

    std::string full_path = dir_path + name;

    struct stat st;
    if (stat(full_path.c_str(), &st) == -1) {
      closedir(dir);
      return -1;
    }

    if (S_ISDIR(st.st_mode)) {
      full_path += "/";
      if (delete_all_directory_content(full_path) != 0) {
        closedir(dir);
        return -1;
      }
      if (rmdir(full_path.c_str()) != 0) {
        closedir(dir);
        return -1;
      }
    } else {
      if (std::remove(full_path.c_str()) != 0) {
        closedir(dir);
        return -1;
      }
    }
  }

  closedir(dir);
  return 0;
}

// autoindex
std::string
HttpRequest::generate_directory_listing(const std::string &dir_path) {
  DIR *dir = opendir(dir_path.c_str());
  if (!dir) {
    return "<html><body><h1>403 Forbidden</h1></body></html>";
  }

  std::ostringstream html;
  html << "<html><head><title>Index of " << dir_path << "</title></head>";
  html << "<body><h1>Index of " << dir_path << "</h1>";
  html << "<ul>";

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    std::string name = entry->d_name;
    if (name == "." || name == "..") {
      continue;
    }
    html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
  }
  html << "</ul></body></html>";
  closedir(dir);

  return html.str();
}

ConnectionPolicy HttpRequest::get_connection_policy() const {
  return connection_policy;
}

void HttpRequest::set_connection_policy(ConnectionPolicy policy) {
  connection_policy = policy;
}

void HttpRequest::set_status_code(int status) {
  if (status_code != 0 && status != 400) {
    return;
  }
  status_code = status;
}

int HttpRequest::get_status_code() const { return status_code; }

size_t HttpRequest::get_max_body_size() const { return max_body_size; }

const std::string &HttpRequest::get_header_value(const std::string &key) const {
  static const std::string k_empty_string;
  ConstHeaderMapIt it = headers.find(key);
  if (it != headers.end() && !it->second.empty()) {
    return it->second.at(0);
  }
  return k_empty_string;
}

const std::vector<std::string> &
HttpRequest::get_header_values(const std::string &key) const {
  static const std::vector<std::string> k_empty_vector;
  ConstHeaderMapIt it = headers.find(key);
  if (it != headers.end()) {
    return it->second;
  }
  return k_empty_vector;
}

void HttpRequest::add_header(const std::string &key, const std::string &value) {

  if (value.empty()) {
    return;
  }

  std::string lower_key = to_lower(key);

  if (lower_key == "date" || key == "set-cookie") {
    headers[lower_key].push_back(trim(value));
    return;
  }

  std::vector<std::string> values = split_csv(value);
  for (size_t i = 0; i < values.size(); ++i) {
    headers[lower_key].push_back(trim(values[i]));
  }
}

bool HttpRequest::is_in_headers(const std::string &key) const {
  return (headers.find(key) != headers.end());
}

void HttpRequest::clear() {
  method.clear();
  path.clear();
  version.clear();
  body_data.clear();
  headers.clear();

  is_autoindex_enabled = false;
  index_file_name.clear();
  cgi_extensions.clear();
  allow_methods.clear();
  error_page_map.clear();

  server_config.clear();
  location_configs.clear();
  best_match_config.clear();
  _root.clear();

  connection_policy = CP_KEEP_ALIVE;
  status_code = 0;
}

HttpRequest &HttpRequest::operator=(const HttpRequest &other) {
  (void)other;
  return *this;
}

RedirStatus HttpRequest::handle_redirection() {

  ConstConfigIt return_it = best_match_config.find("return");
  if (return_it == best_match_config.end()) {
    return REDIR_NONE;
  }
  // return があるが、empty() または `status_code path` 形式でない
  if (return_it->second.empty() || return_it->second.size() != 2) {
    handle_error(400); // TODO: status 要確認
    return REDIR_FAILED;
  }
  int redir_status_code;
  try {
    redir_status_code = str_to_int(return_it->second.at(0));
  } catch (const std::exception &e) {
    log(LOG_ERROR, e.what());
    handle_error(400); // TODO: status 要確認
    return REDIR_FAILED;
  }
  std::string new_location = return_it->second.at(1);
  if (new_location.size() == 0) {
    handle_error(400); // TODO: status 要確認
    return REDIR_FAILED;
  }
  response.generate_redirect(redir_status_code, new_location,
                             connection_policy);
  return REDIR_SUCCESS;
}

void HttpRequest::handle_error(int status_code) {
  if (error_page_map.count(status_code)) {
    const std::string &path = error_page_map[status_code];
    response.generate_custom_error_page(status_code, path, _root,
                                        connection_policy);
  } else {
    response.generate_error_response(status_code, connection_policy);
  }
}

void HttpRequest::set_cgi_handler(CgiHandler* handler) {
  this->cgi = handler;
}
