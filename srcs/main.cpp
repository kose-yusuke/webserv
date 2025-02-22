/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:14 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/02/20 22:33:09 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"
#include "config/config_parse.hpp"

int main(int argc, char  **argv) 
{
    if (argc != 2)
        return (print_error_message("need conf filename"));
    try {
        Parse parser(argv[1]);
        std::vector<std::map<std::string, std::vector<std::string> > > server_configs = parser.parse_nginx_config();
        if (server_configs.empty())
            throw std::runtime_error("No valid server configurations found.");
        std::vector<Server*> servers;
        for (size_t i = 0; i < server_configs.size(); i++)
            servers.push_back(new Server(server_configs[i]));
        for (size_t i = 0; i < servers.size(); i++)
            servers[i]->run();        
        for (size_t i = 0; i < servers.size(); i++)
            delete servers[i];   
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}