/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sakitaha <sakitaha@student.42tokyo.jp>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:05 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/19 22:48:57 by sakitaha         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "Utils.hpp"

// server fd 経由で, server_config, locations_configs を取得
HttpRequest::HttpRequest(int server_fd) {
  Server *server = Multiplexer::get_instance().get_server_from_map(server_fd);
  if (!server) {
    throw std::runtime_error("server not found by HttpRequest");
  }
  this->server_config = server->get_config();
  this->location_configs = server->get_locations();
}

HttpRequest::~HttpRequest() {}

// HttpRequest::HttpRequest(const std::map<std::string, std::vector<std::string>
// >& config, const std::map<std::string, std::map<std::string,
// std::vector<std::string> > >&  location_config) {
//     this->server_configs = config;
//     this->location_configs = location_config;
// }

std::string HttpRequest::handle_http_request(int clientFd, const char *buffer,
                                             int nbytes) {
  (void)nbytes;
  std::string method, path, version;
  // if (!parse_http_request(buffer, method, path, version)) {
  //   HttpResponse::send_error_response(clientFd, 400, "Bad Request");
  //   return;
  // }

  std::cout << "HTTP Method: " << method << ", Path: " << path << "\n";

  // best matchなlocationのconfigを取得する
  ConfigMap config = get_location_config(path);
  this->best_match_location_config = config;

  if (!best_match_location_config["root"].empty())
    _root = best_match_location_config["root"][0];
  else if (!server_config["root"].empty())
    _root = server_config["root"][0];
  else
    print_error_message("No root defound in config file.");

  // autoindex初期化
  is_autoindex_enabled = false;

  // std::map<std::string, std::vector<std::string>>::const_iterator auto_it =
  ConstConfigIt auto_it = config.find("autoindex");
  if (auto_it != config.end() && !auto_it->second.empty() &&
      auto_it->second[0] == "on") {
    is_autoindex_enabled = true;
  }

  // cgi_extensions初期化
  // std::vector<std::string> cgi_extensions = {".cgi", ".php", ".py", ".pl"};
  // std::map<std::string, std::vector<std::string>>::const_iterator
  ConstConfigIt cgi_it = config.find("cgi_extensions");
  if (cgi_it != config.end()) {
    cgi_extensions = cgi_it->second;
  }

  // allow_methods初期化
  // std::map<std::string, std::vector<std::string>>::const_iterator
  ConstConfigIt method_it = config.find("allow_methods");
  if (method_it != config.end()) {
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
      handle_get_request(clientFd, path);
    } else if (method == "POST") {
      handle_post_request(clientFd, buffer, path);
    } else if (method == "DELETE") {
      handle_delete_request(clientFd, path);
    }
  } else {
    HttpResponse::send_error_response(clientFd, 405, "Method Not Allowed");
  }
  return 0;
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

ConfigMap HttpRequest::get_location_config(const std::string &path) {
  ConfigMap selected_config;

  // 最もマッチする `location` を探す
  std::string best_match = "/";
  //   std::map<std::string,
  //            std::map<std::string, std::vector<std::string>>>::const_iterator

  ConstLocationIt best_match_it = location_configs.find("/");

  for (ConstLocationIt it = location_configs.begin();
       it != location_configs.end(); ++it) {
    if (path.find(it->first) == 0 && it->first.length() > best_match.length()) {
      best_match = it->first;
      best_match_it = it;
    }
  }

  // `best_match` に対応する設定を取得
  if (best_match_it != location_configs.end()) {
    selected_config = best_match_it->second;
  }

  return selected_config;
}

/*GET Request*/
void HttpRequest::handle_get_request(int client_socket, std::string path) {

  std::string file_path = get_requested_resource(path);

  if (file_path.empty()) {
    HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
    return;
  }

  ResourceType type = get_resource_type(file_path);

  if (type == Directory) {
    handle_directory_request(client_socket, path);
  } else if (type == File) {
    if (is_cgi_request(file_path))
      handle_cgi_request(client_socket, file_path);
    else
      handle_file_request(client_socket, file_path);
  } else {
    HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
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
void HttpRequest::handle_file_request(int client_socket,
                                      const std::string &file_path) {
  std::ifstream file(file_path.c_str(), std::ios::in);
  if (!file.is_open()) {
    HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
    return;
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  std::string file_content = buffer.str();
  HttpResponse::send_response(client_socket, 200, file_content, "text/html");
}

void HttpRequest::handle_directory_request(int client_socket,
                                           std::string path) {
  // URLの末尾に `/` がない場合、リダイレクト（301）
  if (!ends_with(path, "/")) {
    std::string new_location = path + "/";
    HttpResponse::send_redirect(client_socket, 301, new_location);
    return;
  }

  // `index.html` が存在するか確認 - 本当はこの辺の　public
  // になっているところはrootとかで置き換える必要あり
  if (has_index_file(_root + path)) {
    handle_file_request(client_socket, _root + path + "index.html");
  } else {
    // autoindexがONの場合、ディレクトリリストを生成する
    if (is_autoindex_enabled) {
      std::string dir_listing = generate_directory_listing(_root + path);
      HttpResponse::send_response(client_socket, 200, dir_listing, "text/html");
    } else {
      // 403 forbidden
      HttpResponse::send_custom_error_page(client_socket, 403, "403.html");
    }
  }
}

bool HttpRequest::is_location_upload_file(int client_socket,
                                          const std::string file_path) {
  // 親ディレクトリ取得
  size_t last_slash = file_path.find_last_of('/');
  if (last_slash == std::string::npos) {
    std::cerr << "Invalid file path: " << file_path << std::endl;
    HttpResponse::send_error_response(client_socket, 400, "Bad Request");
    return false;
  }
  std::string parent_dir = file_path.substr(0, last_slash);
  if (!is_directory(parent_dir)) {
    std::cerr << "Parent directory does not exist: " << parent_dir << std::endl;
    HttpResponse::send_error_response(client_socket, 404,
                                      "Parent Directory Not Found");
    return false;
  }

  // 書き込み権限
  if (access(parent_dir.c_str(), W_OK) != 0) {
    HttpResponse::send_error_response(client_socket, 403, "Forbidden");
    return false;
  }

  if (file_exists(file_path)) {
    if (access(file_path.c_str(), W_OK) != 0) {
      HttpResponse::send_error_response(client_socket, 403, "Forbidden");
      return false;
    }
  }

  if (is_cgi_request(file_path))
    return false;
  return true;
}

void HttpRequest::handle_post_request(int client_socket,
                                      const std::string &request,
                                      std::string path) {
  std::string full_path = _root + path;
  if (is_location_upload_file(client_socket, full_path)) {
    std::cout << "Received POST request for path: " << path << std::endl;

    size_t body_start = request.find("\r\n\r\n");
    if (body_start == std::string::npos) {
      // コンテンツなくても特にすることはない？ (そのまま空のファイルを作る？)
      // HttpResponse::send_error_response(client_socket, 204, "No Content");
      // HttpResponse::send_error_response(client_socket, 400, "Bad Request");
      // return;
    }

    std::string body = request.substr(body_start + 4);
    std::cout << "Received POST body: " << body << "\n";

    std::string full_path = "./public" + path;
    std::ofstream ofs(full_path.c_str());
    if (!ofs) {
      // エラーコード確認
      std::cout << "Failed to open file" << std::endl;
      return;
    }
    ofs << body;
    std::cout << "File written successfully: " << path << std::endl;
    ofs.close();

    HttpResponse::send_response(client_socket, 201, body, "text/plain");
  } else {
    // アップロードできない場合は通常のresource取得になるらしい (GETと同様処理)
    handle_get_request(client_socket, path);
  }
}

void HttpRequest::handle_delete_request(int client_socket, std::string path) {
  std::string file_path = get_requested_resource(path);

  if (file_path.empty()) {
    HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
    return;
  }

  ResourceType type = get_resource_type(file_path);

  // 書き込み権限
  if (access(file_path.c_str(), W_OK) != 0) {
    HttpResponse::send_error_response(client_socket, 403, "Forbidden");
    return;
  }

  int status = -1;
  if (type == Directory) {
    delete_directory(file_path);
  } else if (type == File) {
    if (is_cgi_request(file_path))
      handle_cgi_request(client_socket, file_path);
    else {
      status = handle_file_delete(file_path);
      if (status == -1)
        HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
    }
  } else {
    HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
  }

  if (status == 0) {
    HttpResponse::send_response(client_socket, 204, "", "text/plain"); // 成功
  } else {
    HttpResponse::send_error_response(client_socket, 500,
                                      "Internal Server Error"); // 失敗
  }
}

int HttpRequest::handle_file_delete(const std::string &file_path) {
  if (std::remove(file_path.c_str()) == 0) {
    std::cout << "File deleted successfully: " << file_path << std::endl;
    return 0;
  } else {
    std::cerr << "Failed to delete file: " << file_path << std::endl;
    perror("Error");
    return -1;
  }
  return 0;
}

int HttpRequest::delete_directory(const std::string &dir_path) {
  std::string html = dir_path;
  return 0;
}

/*Cgi関連の処理*/
bool HttpRequest::is_cgi_request(const std::string &path) {
  std::string::size_type dot_pos = path.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return false;
  }

  std::string extension = path.substr(dot_pos);
  for (size_t i = 0; i < cgi_extensions.size(); ++i) {
    if (cgi_extensions[i] == extension) {
      return true;
    }
  }
  return false;
  // return (extension == ".cgi" || extension == ".php" || extension == ".py" ||
  // extension == ".pl"); → confファイルで指示あり？
}

void HttpRequest::handle_cgi_request(int client_socket,
                                     const std::string &cgi_path) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    std::cerr << "pipe failed" << std::endl;
    exit(1);
  }

  pid_t pid = fork();
  if (pid < 0) {
    std::cerr << "pipe failed" << std::endl;
    exit(1);
  }

  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    char *argv[2];
    argv[0] = const_cast<char *>(cgi_path.c_str());
    argv[1] = NULL;
    char *envp[] = {NULL};
    execve(cgi_path.c_str(), argv, envp);
    exit(1);
  }

  close(pipefd[1]);
  char buffer[1024];
  std::string cgi_output;
  ssize_t bytes_read;

  while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_read] = '\0';
    cgi_output += buffer;
  }

  close(pipefd[0]);
  waitpid(pid, NULL, 0);

  HttpResponse::send_response(client_socket, 200, cgi_output, "text/html");
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

HttpRequest &HttpRequest::operator=(const HttpRequest &other) {
  (void)other;
  return *this;
}
