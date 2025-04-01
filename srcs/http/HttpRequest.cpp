/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/02 16:37:05 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/04/01 15:27:51 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Multiplexer.hpp"
#include "Server.hpp"
#include "Utils.hpp"

const size_t HttpRequest::k_default_max_body = 104857600;

// server fd 経由で, server_config, locations_configs を取得
HttpRequest::HttpRequest(int server_fd, HttpResponse &httpResponse)
    : response(httpResponse), status_code(0),
        max_body_size(k_default_max_body) {
    Server *server = Multiplexer::get_instance().get_server_from_map(server_fd);
    if (!server) {
        throw std::runtime_error("server not found by HttpRequest");
    }
    this->server_config = server->get_config();
    this->location_configs = server->get_locations();
}

HttpRequest::~HttpRequest() {}

void HttpRequest::init_cgi_extensions() {
    std::map<std::string, std::vector<std::string> >::const_iterator it = best_match_config.find("cgi_extensions");
    if (it != best_match_config.end() && !it->second.empty()) {
        cgi_extensions = it->second;
    }
}

void HttpRequest::conf_init()
{
    is_autoindex_enabled = false;
    this->best_match_config = get_best_match_config(path);
    if (!best_match_config["root"].empty())
        _root = best_match_config["root"][0];
    else if (!server_config["root"].empty())
        _root = server_config["root"][0];
    else
        print_error_message("No root found in config file.");
    init_cgi_extensions();
}

void HttpRequest::handle_http_request() {
    LOG_DEBUG_FUNC();
    conf_init();
    print_best_match_config();

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
        response.generate_error_response(405, "Method Not Allowed");
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
    for (std::map<std::string, std::vector<std::string> >::const_iterator it = override.begin(); it != override.end(); ++it) {
        base[it->first] = it->second;
    }
}

bool regex_match_posix(const std::string& text, const std::string& pattern, bool ignore_case) {
    regex_t regex;
    int cflags = REG_EXTENDED;
    if (ignore_case) cflags |= REG_ICASE;

    if (regcomp(&regex, pattern.c_str(), cflags) != 0)
        return false;

    int result = regexec(&regex, text.c_str(), 0, NULL, 0);
    regfree(&regex);
    return result == 0;
}

ConfigMap HttpRequest::get_best_match_config(const std::string &path) {

    ConfigMap best_config;

    // まず, best_configにserverconfigのdirectiveを代入
    best_config = server_config;

    // 最もマッチする `location` を探す旅にでます
    std::string best_match = "/";

    // 1. 完全一致(= /path) を評価
    for (ConstLocationIt it = location_configs.begin(); it != location_configs.end(); ++it) {
        const std::string& loc = it->first;
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

    for (ConstLocationIt it = location_configs.begin(); it != location_configs.end(); ++it) {
        const std::string& loc = it->first;
        if (loc.substr(0, 3) == "^~ ") {
            std::string clean_loc = loc.substr(3);
            if (path.find(clean_loc) == 0 && clean_loc.length() > longest_prefix.length()) {
                longest_prefix = clean_loc;
                longest_prefix_it = it;
                has_caret_tilde = true;
            }
        } else if (loc[0] != '=' && loc[0] != '~' && path.find(loc) == 0 && loc.length() > longest_prefix.length()) {
            longest_prefix = loc;
            longest_prefix_it = it;
        }
    }

    if (has_caret_tilde) {
        merge_config(best_config, longest_prefix_it->second);
        return best_config;
    }

    // 3. 正規表現マッチを探す
    for (ConstLocationIt it = location_configs.begin(); it != location_configs.end(); ++it) {
        const std::string& loc = it->first;
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
        max_body_size = convert_str_to_size(max_size_str);
    }
    logfd(LOG_DEBUG, "client_max_body_size loaded: ", max_body_size);
}


    /*GET Request*/
void HttpRequest::handle_get_request(std::string path) {

    std::string file_path = get_requested_resource(path);
    if (file_path.empty()) {
        response.generate_custom_error_page(404, "404.html");
        return;
    }

    ResourceType type = get_resource_type(file_path);

    if (type == Directory) {
        handle_directory_request(path);
    } else if (type == File) {
        if (is_cgi_request(file_path))
            handle_cgi_request(file_path);
        else
            handle_file_request(file_path);
    } else {
        response.generate_custom_error_page(404, "404.html");
    }
        if (type == Directory) {
            handle_directory_request(path);
        } else if (type == File) {
            if (is_location_has_cgi() && is_cgi_request(path))
                handle_cgi_request(file_path);
            else
                handle_file_request(file_path);
        }
        else {
            response.generate_custom_error_page(404, "404.html");
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
        response.generate_custom_error_page(404, "404.html");
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string file_content = buffer.str();
    response.generate_response(200, file_content, "text/html");
}

void HttpRequest::handle_directory_request(std::string path) {
    // URLの末尾に `/` がない場合、リダイレクト（301）
    if (!ends_with(path, "/")) {
        std::string new_location = path + "/";
        response.generate_redirect(301, new_location);
        return;
    }

    // `index.html` が存在するか確認 - 本当はこの辺の　public
    // になっているところはrootとかで置き換える必要あり
    if (has_index_file(_root + path)) {
        handle_file_request(_root + path + "index.html");
    } else {
        // autoindexがONの場合、ディレクトリリストを生成する
        if (is_autoindex_enabled) {
        std::string dir_listing = generate_directory_listing(_root + path);
        // HttpResponse::send_response(client_socket, 200, dir_listing,
        // "text/html");
        response.generate_response(200, dir_listing, "text/html");
        } else {
        // 403 forbidden
        // HttpResponse::send_custom_error_page(client_socket, 403, "403.html");
        response.generate_custom_error_page(403, "403.html");
        }
    }
}

bool HttpRequest::is_location_upload_file(const std::string file_path) {
  // 親ディレクトリ取得
    size_t last_slash = file_path.find_last_of('/');
    if (last_slash == std::string::npos) {
        std::cerr << "Invalid file path: " << file_path << std::endl;

        // HttpResponse::send_error_response(client_socket, 400, "Bad Request");
        response.generate_error_response(400, "Bad Request");
        return false;
    }
    std::string parent_dir = file_path.substr(0, last_slash);
    if (!is_directory(parent_dir)) {
        std::cerr << "Parent directory does not exist: " << parent_dir << std::endl;
        // HttpResponse::send_error_response(client_socket, 404, "Parent Directory
        // Not Found");
        response.generate_error_response(404, "Parent Directory Not Found");
        return false;
    }
        // 書き込み権限
        if (access(parent_dir.c_str(), W_OK) != 0) {
            response.generate_error_response(403, "Forbidden");
            return false;
        }

        if (file_exists(file_path)) {
            if (access(file_path.c_str(), W_OK) != 0) {
                response.generate_error_response(403, "Forbidden");
                return false;
            }
        }

        return true;
}

void HttpRequest::handle_post_request() {
    std::string full_path = _root + path;

    std::cout << path  << std::endl;
    std::cout << is_location_has_cgi()  << std::endl;
    std::cout << is_cgi_request(path) << std::endl;

    if (is_location_has_cgi() && is_cgi_request(path)) {
        std::cout << "test  - test"  << std::endl;
        handle_cgi_request(full_path);
        return;
    }

    if (!is_location_upload_file(full_path)) {
        handle_get_request(path); // POSTが許されない場所ならGETにフォールバック
        return;
    }

    // すでに HttpRequestParserがbodyをrequestから分離済みのため, コメントアウト
    // size_t body_start = request.find("\r\n\r\n");
    // if (body_start == std::string::npos) {
    //     response.generate_error_response(400, "Bad Request: No Header-Body separator");
    //     return;
    // }

    // TODO:
    // 変更を最小限にするため、一旦 stringに変換している
    // バイナリをbodyで受け取る必要があると思うので、要修正
    // std::string body = request.substr(body_start + 4);
    
    std::string body(body_data.begin(), body_data.end());
    std::cout << "Received POST body: " << body << std::endl;

    if (body.empty()) {
        response.generate_response(204, "", "text/plain");
        return;
    }

    std::string upload_path = "./public" + path;
    std::ofstream ofs(upload_path.c_str());
    if (!ofs) {
        std::cerr << "Failed to open file: " << upload_path << std::endl;
        response.generate_error_response(500, "Internal Server Error: Failed to open file");
        return;
    }

    ofs << body;
    ofs.close();

    std::cout << "File written successfully: " << path << std::endl;
    response.generate_response(201, body, "text/plain");
}

void HttpRequest::handle_delete_request(const std::string path) {
    std::string file_path = get_requested_resource(path);

    if (file_path.empty()) {
        // HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
        response.generate_custom_error_page(404, "404.html");
        return;
    }

    ResourceType type = get_resource_type(file_path);

    // 書き込み権限
    if (access(file_path.c_str(), W_OK) != 0) {
        // HttpResponse::send_error_response(client_socket, 403, "Forbidden");
        response.generate_error_response(403, "Forbidden");
        return;
    }

    int status = -1;
    if (type == Directory) {
        handle_directory_delete(file_path);
    } else if (type == File) {
        if (is_cgi_request(file_path))
            handle_cgi_request(file_path);
        else {
            status = handle_file_delete(file_path);
        if (status == -1)
            response.generate_custom_error_page(404, "404.html");
        }
    } else {
        // HttpResponse::send_custom_error_page(client_socket, 404, "404.html");
        response.generate_custom_error_page(404, "404.html");
    }

    if (status == 0) {
        // HttpResponse::send_response(client_socket, 204, "", "text/plain");
        response.generate_response(204, "", "text/plain"); // 成功
    } else {
        // HttpResponse::send_error_response(client_socket, 500,
        //                              "Internal Server Error");
        // 失敗
        response.generate_error_response(500, "Internal Server Error");
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

int HttpRequest::handle_directory_delete(const std::string& dir_path) {
    if (!ends_with(dir_path, "/")) {
        response.generate_error_response(409, "Conflict");
        return -1;
    }

    std::string html = dir_path;
    return 0;
}

int HttpRequest::delete_all_directory_content(){
    return 1;
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

void HttpRequest::print_best_match_config() const {
    std::cout << "=== best_match_config ===" << std::endl;
    for (std::map<std::string, std::vector<std::string> >::const_iterator it = best_match_config.begin();
         it != best_match_config.end(); ++it) {
        std::cout << "Key: " << it->first << std::endl;
        std::cout << "Values:";
        for (std::vector<std::string>::const_iterator vit = it->second.begin();
             vit != it->second.end(); ++vit) {
            std::cout << " " << *vit;
        }
        std::cout << std::endl;
    }
    std::cout << "==================================" << std::endl;
}

bool HttpRequest::is_location_has_cgi() {
    std::map<std::string, std::vector<std::string> >::const_iterator it = best_match_config.find("cgi_extensions");
    if (it == best_match_config.end() || it->second.empty())
        return false;
    return true;
}

void HttpRequest::handle_cgi_request(const std::string& cgi_path) {
    int input_pipe[2], output_pipe[2];
    if (pipe(input_pipe) == -1)
    {
        std::cerr << "pipe failed" << std::endl;
        std::exit(1);
    }

    if (pipe(output_pipe) == -1)
    {
        std::cerr << "pipe failed" << std::endl;
        std::exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "fork failed" << std::endl;
        std::exit(1);
    }

    if (pid == 0) {
        dup2(input_pipe[0], STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        close(input_pipe[1]);
        close(output_pipe[0]);

        std::string contentLength = get_value_from_headers("Content-Length");;
        std::string contentLengthStr = "CONTENT_LENGTH=" + contentLength;
        std::string requestMethodStr = "REQUEST_METHOD=POST";
        std::string contentTypeStr = "CONTENT_TYPE="+ get_value_from_headers("Content-Type");
        std::string queryString = "QUERY_STRING=";

        if (method == "POST") {
            std::string body(body_data.begin(), body_data.end());
            queryString += body;
        }
        else if (method == "GET") {
            size_t pos = path.find('?');
            if (pos != std::string::npos) {
                queryString += path.substr(pos + 1);
            }
        }
        (void)cgi_path;

        char *envp[] = {
            const_cast<char *>(requestMethodStr.c_str()),
            const_cast<char *>(contentLengthStr.c_str()),
            const_cast<char *>(contentTypeStr.c_str()),
            const_cast<char *>(queryString.c_str()),
            NULL
        };
        
        char *argv[] = { const_cast<char *>(cgi_path.c_str()), NULL };

        execve(cgi_path.c_str(), argv, envp);
        perror("execve");
        std::exit(1);
    } else {
        close(input_pipe[0]);
        // TODO: String body -> Vecotr int body_dataのため
        // 応急処置としてここでstringにしている
        std::string body(body_data.begin(), body_data.end());
        write(input_pipe[1], body.c_str(), body.size());
        close(input_pipe[1]);

        close(output_pipe[1]);
        std::string cgi_output;
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(output_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            cgi_output += buffer;
        }
        close(output_pipe[0]);
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            response.generate_response(200, cgi_output, "text/html");
        } else {
            response.generate_error_response(500, "CGI Execution Failed");
        }
    }
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

void HttpRequest::set_status_code(int status) { status_code = status; }

int HttpRequest::get_status_code() const { return status_code; }

size_t HttpRequest::get_max_body_size() const { return max_body_size; }

bool HttpRequest::is_in_headers(const std::string &key) const {
  return (headers.find(key) != headers.end());
}

std::string HttpRequest::get_value_from_headers(const std::string &key) const {
  ConstStrToStrMapIt it = headers.find(key);
  if (it != headers.end()) {
    return it->second;
  }
  return "";
}

bool HttpRequest::add_header(std::string &key, std::string &value) {
    if (headers.find(key) != headers.end()) {
        return false;
    }
    headers[key] = value;
    return true;
}

void HttpRequest::clear() {
    method.clear();
    path.clear();
    version.clear();
    body_data.clear();
    headers.clear();
    status_code = 0;
}

HttpRequest &HttpRequest::operator=(const HttpRequest &other) {
    (void)other;
    return *this;
}

// bool HttpRequest::parse_http_request(const std::string &request,
//                                      std::string &method, std::string
//                                      &path, std::string &version) {
//   std::istringstream request_stream(request);
//   if (!(request_stream >> method >> path >> version)) {
//     return false;
//   }
//   return true;
// }
