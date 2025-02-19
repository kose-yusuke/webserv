/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_parse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/19 19:24:27 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/19 21:44:19 by koseki.yusu      ###   ########.fr       */
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
        std::vector<std::map<std::string, std::string> > parse_nginx_config(); 
        void handle_server_block(const std::string& line, std::map<std::string, std::string>& current_config, bool& in_location_block);
        void process_line(std::string& line, std::map<std::string, std::string>& current_config, bool& in_server_block, bool& in_location_block);
        std::string space_outer_trim(const std::string& str);
};