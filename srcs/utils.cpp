#include "../includes/webserv.hpp"

// ファイルの内容を読み取るヘルパー関数
std::string read_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::in);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 拡張子に基づいてMIMEタイプを返す関数
// std::string get_mime_type(const std::string& file_path) {
//     // 拡張子とMIMEタイプのマッピング
//     static const std::map<std::string, std::string> mime_types = {
//         {".html", "text/html"},
//         {".css", "text/css"},
//         {".js", "application/javascript"},
//         {".png", "image/png"},
//         {".jpg", "image/jpeg"},
//         {".jpeg", "image/jpeg"},
//         {".gif", "image/gif"},
//         {".svg", "image/svg+xml"},
//         {".json", "application/json"},
//         {".txt", "text/plain"}
//     };

//     // ファイルパスから拡張子を抽出
//     size_t dot_pos = file_path.find_last_of('.');
//     if (dot_pos != std::string::npos) {
//         std::string extension = file_path.substr(dot_pos);
//         if (mime_types.find(extension) != mime_types.end()) {
//             return mime_types.at(extension);
//         }
//     }

//     // デフォルトのMIMEタイプ
//     return "application/octet-stream";
// }


void send_custom_error_page(int client_socket, int status_code, const std::string& error_page) 
{
    try {
        // エラーページのコンテンツを読み取る
        std::string file_content = read_file("./srcs/public/" + error_page);

        // HTTPレスポンスの生成
        std::ostringstream response;
        response << "HTTP/1.1 " << status_code << " ";
        if (status_code == 404) {
            response << "Not Found";
        } else if (status_code == 405) {
            response << "Method Not Allowed";
        }
        response << "\r\n";
        response << "Content-Length: " << file_content.size() << "\r\n";
        response << "Content-Type: text/html\r\n\r\n";
        response << file_content;

        send(client_socket, response.str().c_str(), response.str().size(), 0);
    } catch (const std::exception& e) {
        // エラーページが存在しない場合、簡易メッセージを返す
        std::ostringstream fallback;
        fallback << "HTTP/1.1 " << status_code << " ";
        if (status_code == 404) {
            fallback << "Not Found";
        } else if (status_code == 405) {
            fallback << "Method Not Allowed";
        }
        fallback << "\r\nContent-Length: 9\r\n\r\nNot Found";
        send(client_socket, fallback.str().c_str(), fallback.str().size(), 0);
    }
}


// Nginx形式の設定ファイルを読み取る関数
// トリム関数
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

// Nginx形式の設定ファイルを読み取る関数
std::map<std::string, std::string> parse_nginx_config(const std::string& config_path) {
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_path);
    }

    std::map<std::string, std::string> config;
    std::string line;
    bool in_server_block = false;

    while (std::getline(config_file, line)) {
        // コメントと空行をスキップ
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        line = trim(line);  // 行全体をトリム
        if (line.empty()) {
            continue;
        }

        // ブロックの開始/終了を検出
        if (line == "server {") {
            in_server_block = true;
            continue;
        } else if (line == "}" && in_server_block) {
            break;
        }

        if (in_server_block) {
            // セミコロンで終わる行を処理
            size_t semicolon_pos = line.find(';');
            if (semicolon_pos == std::string::npos) {
                throw std::runtime_error("Invalid config line: " + line);
            }

            std::string key_value = line.substr(0, semicolon_pos);
            key_value = trim(key_value);

            // key と value を空白で分離
            size_t space_pos = key_value.find(' ');
            if (space_pos == std::string::npos) {
                throw std::runtime_error("Invalid config line: " + line);
            }

            std::string key = key_value.substr(0, space_pos);
            std::string value = key_value.substr(space_pos + 1);

            // トリムを適用
            key = trim(key);
            value = trim(value);

            // マップに追加
            config[key] = value;

            // デバッグ用出力
            std::cout << "Parsed config: key='" << key << "', value='" << value << "'\n";
        }
    }

    if (!in_server_block) {
        throw std::runtime_error("No server block found in config file.");
    }

    return config;
}

