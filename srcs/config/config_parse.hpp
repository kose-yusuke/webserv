/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_parse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/19 19:24:27 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/20 18:21:19 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../includes/webserv.hpp"

class Parse{
    private:
        std::string _config_path;
        static const char* valid_keys[];
        static const char* required_keys[];
        std::vector<std::map<std::string, std::string> > server_configs; 
        
    public:

        Parse();
        Parse(std::string config_path);
        ~Parse();
        Parse(const Parse &src);
        Parse &operator=(const Parse &src);
        
        void validate_config(const std::map<std::string, std::string>& config); 
        void validate_config_keys(const std::map<std::string, std::string>& config); 
        void validate_location_path(const std::map<std::string, std::string>& config);
        void validate_server_name(const std::map<std::string, std::string>& config);
        void validate_listen_port(const std::map<std::string, std::string>& config);
        void validate_listen_ip(const std::map<std::string, std::string>& config);
        std::vector<std::map<std::string, std::string> > parse_nginx_config(); 
        void handle_server_block(const std::string& line, std::map<std::string, std::string>& current_config, std::map<std::string, std::map<std::string, std::string> >& location_configs, bool& in_location_block, std::string& current_location_path, bool& server_root_seen);
        void process_line(std::string& line, std::map<std::string, std::string>& current_config, std::map<std::string, std::map<std::string, std::string> >& location_configs, bool& in_server_block, bool& in_location_block,std::string& current_location_path, bool& server_root_seen);
        void reset_server_config(std::map<std::string, std::string>& current_config,std::map<std::string, std::map<std::string, std::string> >& location_configs,bool& server_root_seen);
        bool is_server_start(const std::string& line);
        bool is_server_end(const std::string& line, bool in_server_block, bool in_location_block);
        bool is_location_start(const std::string& line);
        bool is_location_end(const std::string& line, bool in_location_block);
        void parse_key_value(const std::string& line, std::string& key, std::string& value);
        void check_duplicate_key(const std::string& key, const std::map<std::string, std::string>& config);
        std::string space_outer_trim(const std::string& str);
};