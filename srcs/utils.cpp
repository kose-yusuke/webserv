/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/11/18 15:47:21 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/03/02 16:30:41 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../includes/webserv.hpp"

int print_error_message(const std::string& message)
{
    std::cerr << "Error: " << message << std::endl;
    return (1);
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
