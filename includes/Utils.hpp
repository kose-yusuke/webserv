/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/05 21:09:37 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/05 21:09:39 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Logger.hpp"
#include "types.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <regex.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <ctime>

std::string read_file(const std::string &file_path);
int print_error_message(const std::string &message);
void print_best_match_config(ConfigMap best_match_config);

// utils
bool file_exists(const std::string &path);
bool ends_with(const std::string &str, const std::string &suffix);
bool has_index_file(const std::string &dir_path, std::string index_file_name);
bool is_directory(const std::string &path);
std::string make_unique_filename();

// trim string
std::string trim_left(const std::string &s);
std::string trim_right(const std::string &s);
std::string trim(const std::string &s);

// convert string to number
int str_to_int(const std::string &s);
size_t str_to_size(const std::string &s);
size_t parse_hex(const std::string &s);

std::vector<std::string> split_csv(const std::string &value);
std::string to_lower(const std::string &s);
bool regex_match_posix(const std::string &text, const std::string &pattern, bool ignore_case);

bool is_all_digits(const std::string &str);
std::string getExtension(const std::string& path);
std::string get_date();
