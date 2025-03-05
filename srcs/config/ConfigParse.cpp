/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:45:57 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/05 21:15:15 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// #include "../../includes/webserv.hpp"
#include "Utils.hpp"
#include "ConfigParse.hpp"

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

/* Validateに関するコード*/
const char* Parse::valid_keys[] = {
    "listen", "root", "index", "error_page", "autoindex", "server_name", "allow_methods", "client_max_body_size", "return", "cgi_extension", "upload_path", "alias"
};


// rootはlocationにあればよさそう
const char* Parse::required_keys[] = {
    "listen"
};

void Parse::validate_config(const std::map<std::string, std::vector<std::string> >& config)
{
    validate_config_keys(config);
    validate_listen_ip(config);
    validate_listen_port(config);
    validate_server_name(config);
    validate_location_path(config);
}

void Parse::validate_config_keys(const std::map<std::string, std::vector<std::string> >& config)
{
    for (size_t i = 0; i < sizeof(required_keys) / sizeof(required_keys[0]); i++) {
        if (config.find(required_keys[i]) == config.end()) {
            throw std::runtime_error(std::string("Missing required key: ") + required_keys[i]);
        }
    }

    for (std::map<std::string, std::vector<std::string> >::const_iterator it = config.begin(); it != config.end(); ++it) {
        bool is_valid = false;
        for (size_t i = 0; i < sizeof(valid_keys) / sizeof(valid_keys[0]); i++) {
            if (it->first == valid_keys[i]) {
                is_valid = true;
                break;
            }
        }
        if (!is_valid) {
            throw std::runtime_error("Unknown key found: " + it->first);
        }
    }
}


void Parse::validate_location_path(const std::map<std::string, std::vector<std::string> >& config)
{
    std::vector<std::string> seen_locations;
    for (std::map<std::string, std::vector<std::string> >::const_iterator it = config.begin(); it != config.end(); ++it) {
        if (it->first.find("location") == 0) {
            for (size_t i = 0; i < seen_locations.size(); i++) {
                if (seen_locations[i] == it->first) {
                    throw std::runtime_error("Duplicate location block: " + it->first);
                }
            }
            seen_locations.push_back(it->first);
        }
    }
}

void Parse::validate_server_name(const std::map<std::string, std::vector<std::string> >& config)
{
    static std::vector<std::string> seen_server_names;

    std::map<std::string, std::vector<std::string> >::const_iterator it = config.find("server_name");
    if (it != config.end()) {
        for (size_t j = 0; j < it->second.size(); j++) {
            for (size_t i = 0; i < seen_server_names.size(); i++) {
                if (seen_server_names[i] == it->second[j])
                    throw std::runtime_error("Duplicate server_name found: " + it->second[j]);
            }
            seen_server_names.push_back(it->second[j]);
        }
    }
}

bool is_valid_ip(const std::string& ip)
{
    int dots = 0;
    int num = 0;
    bool has_digit = false;

    for (size_t i = 0; i < ip.length(); i++) {
        if (std::isdigit(ip[i])) {
            num = num * 10 + (ip[i] - '0');
            if (num > 255)
                return false;
            has_digit = true;
        } else if (ip[i] == '.') {
            if (!has_digit)
                return false;
            dots++;
            num = 0;
            has_digit = false;
        } else {
            return false;
        }
    }
    return (dots == 3 && has_digit);
}

void Parse::validate_listen_ip(const std::map<std::string, std::vector<std::string> >& config)
{
    std::map<std::string, std::vector<std::string> >::const_iterator it = config.find("listen");
    if (it != config.end()) {
        for (size_t j = 0; j < it->second.size(); j++) {
            std::string listen_value = space_outer_trim(it->second[j]);
            size_t colon_pos = listen_value.find(':');
            size_t dot_pos = listen_value.find('.');

            if (colon_pos != std::string::npos) {
                std::string ip = listen_value.substr(0, colon_pos);
                std::string port = listen_value.substr(colon_pos + 1);

                if (!is_valid_ip(ip))
                    throw std::runtime_error("Invalid IP address in listen directive: " + ip);
            } else if (dot_pos != std::string::npos) {
                if (!is_valid_ip(listen_value))
                    throw std::runtime_error("Invalid IP address in listen directive: " + listen_value);
            }
        }
    }
}


void Parse::validate_listen_port(const std::map<std::string, std::vector<std::string> >& config)
{
    std::map<std::string, std::vector<std::string> >::const_iterator it = config.find("listen");
    if (it != config.end()) {
        for (size_t j = 0; j < it->second.size(); j++) {
            std::string port_str;
            size_t colon_pos = it->second[j].find(':');

            if (colon_pos != std::string::npos) {
                port_str = it->second[j].substr(colon_pos + 1);
            } else {
                port_str = it->second[j];
            }

            std::stringstream ss(port_str);
            int port;
            if (!(ss >> port) || port < 1 || port > 65535) {
                throw std::runtime_error("Invalid port number in listen directive: " + port_str);
            }
        }
    }
}

bool Parse::is_server_start(const std::string& line) {
    return line == "server {";
}

bool Parse::is_server_end(const std::string& line, bool in_server_block, bool in_location_block) {
    return line == "}" && in_server_block && !in_location_block;
}

bool Parse::is_location_start(const std::string& line) {
    return line.find("location") == 0 && line.find('{') != std::string::npos;
}

bool Parse::is_location_end(const std::string& line, bool in_location_block) {
    return line == "}" && in_location_block;
}

void Parse::check_duplicate_key(const std::string& key, const std::map<std::string, std::string>& config) {
    if (config.find(key) != config.end())
        throw std::runtime_error("Duplicate key found: " + key);
}

/* Parser */
std::vector<std::map<std::string, std::vector<std::string> > > Parse::parse_nginx_config()
{
    std::ifstream _config_file(_config_path.c_str());
    if (!_config_file.is_open())
        throw std::runtime_error("Failed to open config file: " + _config_path);

    std::string line;
    bool in_server_block = false;
    bool in_location_block = false;
    bool found_server_block = false;
    bool server_root_seen = false;
    std::string current_location_path = "";

    std::vector<std::map<std::string, std::vector<std::string> > > server_configs;
    std::map<std::string, std::vector<std::string> > current_config;
    std::map<std::string, std::map<std::string, std::string> > location_configs;


    while (std::getline(_config_file, line))
    {
        process_line(line, current_config, location_configs, in_server_block, in_location_block, current_location_path, server_root_seen);
        if (in_server_block)
            found_server_block = true;
        if (!in_server_block && !current_config.empty()) {
            validate_config(current_config);
            server_configs.push_back(current_config);
            reset_server_config(current_config, location_configs, server_root_seen);
        }
    }

    if (!found_server_block) {
        throw std::runtime_error("No server block found in config file.");
    }
    return server_configs;
}

void Parse::process_line(std::string& line, std::map<std::string, std::vector<std::string> >& current_config, std::map<std::string, std::map<std::string, std::string> >& location_configs, bool& in_server_block, bool& in_location_block,std::string& current_location_path, bool& server_root_seen)
{
    size_t comment_pos = line.find('#');
    if (comment_pos != std::string::npos)
        line = line.substr(0, comment_pos);
    line = space_outer_trim(line);
    if (line.empty()) {
        return;
    }

    if (is_server_start(line)) {
        if (in_server_block)
            throw std::runtime_error("Nested server blocks are not allowed.");
        in_server_block = true;
        return;
    } else if (is_server_end(line, in_server_block, in_location_block)) {
        in_server_block = false;
        return;
    }

    if (in_server_block)
        handle_server_block(line, current_config, location_configs, in_location_block, current_location_path, server_root_seen);
    else
        throw std::runtime_error("Invalid config structure: No active server block.");
}

void Parse::handle_server_block(const std::string& line, std::map<std::string, std::vector<std::string> >& current_config, std::map<std::string, std::map<std::string, std::string> >& location_configs, bool& in_location_block, std::string& current_location_path, bool& server_root_seen)
{
    if (is_location_start(line)) {
        size_t pos = line.find('{');
        if (pos == std::string::npos) {
            throw std::runtime_error("Invalid location block format: " + line);
        }

        std::string location_path = space_outer_trim(line.substr(9, pos - 9));
        if (location_configs.find(location_path) != location_configs.end()) {
            throw std::runtime_error("Duplicate location block: " + location_path);
        }

        in_location_block = true;
        current_location_path = location_path;
        location_configs[current_location_path] = std::map<std::string, std::string>();
        return;
    }
    else if (is_location_end(line, in_location_block)) {
        in_location_block = false;
        current_location_path = "";
        return;
    }

    size_t semicolon_pos = line.find(';');
    if (semicolon_pos == std::string::npos) {
        throw std::runtime_error("Invalid config line: " + line);
    }

    std::string key, value;
    parse_key_value(line, key, value);

    key = space_outer_trim(key);
    value = space_outer_trim(value);

    if (in_location_block) {
        check_duplicate_key(key, location_configs[current_location_path]);
        location_configs[current_location_path][key] = value;
    } else {
        if (key == "root" && server_root_seen)
            throw std::runtime_error("Duplicate root directive found in server block.");
        else if (key == "root")
            server_root_seen = true;
        current_config[key].push_back(value);
    }
    std::cout << "Parsed config: key='" << key << "', value='" << value << "'\n";
}

/* parser utils */

void Parse::parse_key_value(const std::string& line, std::string& key, std::string& value) {
    size_t semicolon_pos = line.find(';');
    if (semicolon_pos == std::string::npos)
        throw std::runtime_error("Invalid config line: " + line);

    std::string key_value = space_outer_trim(line.substr(0, semicolon_pos));
    size_t space_pos = key_value.find(' ');
    if (space_pos == std::string::npos)
        throw std::runtime_error("Invalid config line: " + line);

    key = space_outer_trim(key_value.substr(0, space_pos));
    value = space_outer_trim(key_value.substr(space_pos + 1));
}

void Parse::reset_server_config(std::map<std::string, std::vector<std::string> >& current_config,
                                std::map<std::string, std::map<std::string, std::string> >& location_configs,
                                bool& server_root_seen) {
    current_config.clear();
    location_configs.clear();
    server_root_seen = false;
}

std::string Parse::space_outer_trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}
