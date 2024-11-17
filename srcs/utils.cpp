#include "../includes/webserv.hpp"
#include <map>

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
