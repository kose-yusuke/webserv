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