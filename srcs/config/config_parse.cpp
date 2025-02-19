/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_parse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:45:57 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/19 21:44:44 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include "../../includes/webserv.hpp"
#include "config_parse.hpp"

Parse::Parse(){}

Parse::Parse(std::string config_path)
{
    _config_path = config_path;
}

Parse::~Parse(){}

Parse::Parse(const Parse &src) 
{
    this->_config_path = src._config_path;
}

Parse& Parse::operator=(const Parse &src)
{
    if (this != &src)
    {
        _config_path = src._config_path;
    }
    return (*this);
}

std::string Parse::space_outer_trim(const std::string& str) 
{
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

const char* Parse::valid_keys[] = {
    "listen", "root", "error_page", "autoindex"
};

const char* Parse::required_keys[] = {
    "listen", "root"
};

void Parse::validate_config(const std::map<std::string, std::string>& config)
{
    validate_config_keys(config);
}

void Parse::validate_config_keys(const std::map<std::string, std::string>& config) {
    for (size_t i = 0; i < sizeof(required_keys) / sizeof(required_keys[0]); i++) {
        if (config.find(required_keys[i]) == config.end()) {
            throw std::runtime_error(std::string("Missing required key: ") + required_keys[i]);
        }
    }

    for (std::map<std::string, std::string>::const_iterator it = config.begin(); it != config.end(); ++it) {
        bool is_valid = false;
        for (size_t i = 0; i < sizeof(valid_keys) / sizeof(valid_keys[0]); i++) {
            if (it->first == valid_keys[i]) {
                is_valid = true;
                break;
            }
        }
        if (!is_valid) {
            throw std::runtime_error(std::string("Unknown key found: ") + it->first);
        }
    }
}

// Nginx形式の設定ファイルを読み取る関数
// serverが複数ある場合に対応できていない
// invalidのconfigに対応できていない → エラーハンドリング
std::vector<std::map<std::string, std::string> > Parse::parse_nginx_config()
{
    std::ifstream _config_file(_config_path);
    if (!_config_file.is_open())
        throw std::runtime_error("Failed to open config file: " + _config_path);
    
    std::string line;
    bool in_server_block = false;
    bool in_location_block = false;
    bool found_server_block = false;

    std::vector<std::map<std::string, std::string> > server_configs;
    std::map<std::string, std::string> current_config;


    while (std::getline(_config_file, line)) 
    {
        process_line(line, current_config, in_server_block, in_location_block);
        if (in_server_block)
            found_server_block = true;
        if (!in_server_block && !current_config.empty()) {
            validate_config(current_config);
            server_configs.push_back(current_config);
            current_config.clear();
        }
    }
    
    if (!found_server_block) {
        throw std::runtime_error("No server block found in config file.");
    }
    return server_configs;
}

void Parse::process_line(std::string& line, std::map<std::string, std::string>& current_config, bool& in_server_block, bool& in_location_block)
{
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos) 
        line = line.substr(0, comment_pos);
    line = space_outer_trim(line);
    if (line.empty()) {
        return;
    }

    if (line == "server {") {
        if (in_server_block)
            throw std::runtime_error("Nested server blocks are not allowed.");
        in_server_block = true;
        return;
    } else if (line == "}" && in_server_block && !in_location_block) {
        in_server_block = false;
        return;
    } 
        
    if (in_server_block) 
        handle_server_block(line, current_config, in_location_block);
    else
        throw std::runtime_error("Invalid config structure: No active server block.");
}

void Parse::handle_server_block(const std::string& line, std::map<std::string, std::string>& current_config, bool& in_location_block)
{
    if (line == "location / {") {
        in_location_block = true;
        return;
    } else if (line == "}" && in_location_block) {
        in_location_block = false;
        return;
    }

    size_t semicolon_pos = line.find(';');
    if (semicolon_pos == std::string::npos) {
        throw std::runtime_error("Invalid config line: " + line);
    }

    std::string key_value = line.substr(0, semicolon_pos);
    key_value = space_outer_trim(key_value);

    size_t space_pos = key_value.find(' ');
    if (space_pos == std::string::npos) {
        throw std::runtime_error("Invalid config line: " + line);
    }

    std::string key = key_value.substr(0, space_pos);
    std::string value = key_value.substr(space_pos + 1);

    key = space_outer_trim(key);
    value = space_outer_trim(value);
    
    current_config[key] = value;

    std::cout << "Parsed config: key='" << key << "', value='" << value << "'\n";
}

