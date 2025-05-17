/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CgiHandler.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: koseki.yusuke <koseki.yusuke@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/17 18:32:44 by koseki.yusu       #+#    #+#             */
/*   Updated: 2025/05/17 19:22:55 by koseki.yusu      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiHandler.hpp"

CgiHandler::CgiHandler(HttpResponse &httpResponse, HttpRequest &httpRequest) : 
    response(httpResponse), request(httpRequest) {}

CgiHandler::~CgiHandler() {}

// CgiHandler::CgiHandler(const CgiHandler &src){

// }

CgiHandler &CgiHandler::operator=(const CgiHandler &other) {
    (void)other;
    return *this;
}

bool CgiHandler::is_cgi_request(const std::string &path, const std::vector<std::string>& cgi_extensions) {
    std::string::size_type dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos)
        return false;

    std::string extension = path.substr(dot_pos);
    for (size_t i = 0; i < cgi_extensions.size(); ++i) {
        if (cgi_extensions[i] == extension)
            return true;
    }
    return false;
}

bool CgiHandler::is_location_has_cgi(ConfigMap best_match_config) {
    ConstConfigIt it = best_match_config.find("cgi_extensions");
    if (it == best_match_config.end() || it->second.empty())
        return false;
    return true;
}

void CgiHandler::handle_cgi_request(const std::string &cgi_path, std::vector<char> body_data, std::string method, std::string path) {
    int output_pipe[2];

    if (pipe(output_pipe) == -1) {
        std::cerr << "pipe failed" << std::endl;
        std::exit(1);
    }

    // TODO: String body -> Vector int body_dataのため
    // 応急処置としてここでstringにしている
    std::string body(body_data.begin(), body_data.end());

    std::string tmp_path;
    int tmp_fd = -1;
    for (int i = 0; i < 10; ++i) {
        tmp_path = "/tmp/cgi_tmp_" + make_unique_filename();
        tmp_fd = open(tmp_path.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
        if (tmp_fd != -1)
        break;
    }
    if (tmp_fd == -1) {
        std::cerr << "Failed to create temp file: " << strerror(errno) << std::endl;
        std::exit(1);
    }

    std::ofstream ofs(tmp_path.c_str());
    if (!ofs) {
        std::cerr << "Failed to open temp file for writing" << std::endl;
        close(tmp_fd);
        std::exit(1);
    }
    ofs << body;
    ofs.close();
    close(tmp_fd);

    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "fork failed" << std::endl;
        std::exit(1);
    }

    if (pid == 0) {
        int in_fd = open(tmp_path.c_str(), O_RDONLY);
        if (in_fd == -1) {
        std::cerr << "open temp file failed: " << strerror(errno) << std::endl;
        std::exit(1);
        }

        dup2(in_fd, STDIN_FILENO);
        dup2(output_pipe[1], STDOUT_FILENO);
        close(in_fd);
        close(output_pipe[0]);

        std::string contentLength = request.get_header_value("Content-Length");
        std::string contentLengthStr = "CONTENT_LENGTH=" + contentLength;
        std::string requestMethodStr = "REQUEST_METHOD=POST";
        std::string contentTypeStr =
            "CONTENT_TYPE=" + request.get_header_value("Content-Type");
        std::string queryString = "QUERY_STRING=";

        if (method == "POST") {
        std::string body(body_data.begin(), body_data.end());
        queryString += body;
        } else if (method == "GET") {
        size_t pos = path.find('?');
        if (pos != std::string::npos) {
            queryString += path.substr(pos + 1);
        }
        }
        (void)cgi_path;

        char *envp[] = {const_cast<char *>(requestMethodStr.c_str()),
                        const_cast<char *>(contentLengthStr.c_str()),
                        const_cast<char *>(contentTypeStr.c_str()),
                        const_cast<char *>(queryString.c_str()), NULL};

        char *argv[] = {const_cast<char *>(cgi_path.c_str()), NULL};

        execve(cgi_path.c_str(), argv, envp);
        perror("execve");
        std::exit(1);
    } else {
        close(output_pipe[1]);

        std::string cgi_output;
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(output_pipe[0], buffer, sizeof(buffer) - 1)) >
                0) {
        buffer[bytes_read] = '\0';
        cgi_output += buffer;
        }
        close(output_pipe[0]);
        int status;
        waitpid(pid, &status, 0);

        std::remove(tmp_path.c_str());

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        response.generate_response(200, cgi_output, "text/html",
                                    request.get_connection_policy());
        } else {
        response.generate_error_response(500, "CGI Execution Failed",
            request.get_connection_policy());
        }
    }
}