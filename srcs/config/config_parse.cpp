/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_parse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:45:57 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/15 19:16:32 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/webserv.hpp"

// Nginx形式の設定ファイルを読み取る関数
// トリム関数
std::string space_outer_trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

// Nginx形式の設定ファイルを読み取る関数
std::map<std::string, std::string> parse_nginx_config(const std::string& config_path) 
{
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_path);
    }

    std::map<std::string, std::string> config;
    std::string line;
    bool in_server_block = false;
    bool in_location_block = false;

    while (std::getline(config_file, line)) 
    {
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) 
        {
            // #(コメントアウト)以降をNULLに置き換える
            line = line.substr(0, comment_pos);
        }
        line = space_outer_trim(line);
        if (line.empty()) {
            continue;
        }

        if (line == "server {") {
            in_server_block = true;
            continue;
        } else if (line == "}" && in_server_block && !in_location_block) {
            in_server_block = false;
            break;
        }

        if (in_server_block) 
        {    
            if (line == "location / {") {
                in_location_block = true;
                continue;
            } else if (line == "}" && in_location_block) {
                in_location_block = false;
                break;
            }

            // セミコロンで終わる行を処理
            size_t semicolon_pos = line.find(';');
            if (semicolon_pos == std::string::npos) {
                throw std::runtime_error("Invalid config line: " + line);
            }

            std::string key_value = line.substr(0, semicolon_pos);
            key_value = space_outer_trim(key_value);

            // keyとvalueを空白で分離
            size_t space_pos = key_value.find(' ');
            if (space_pos == std::string::npos) {
                throw std::runtime_error("Invalid config line: " + line);
            }

            std::string key = key_value.substr(0, space_pos);
            std::string value = key_value.substr(space_pos + 1);

            // トリムを適用
            key = space_outer_trim(key);
            value = space_outer_trim(value);

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

