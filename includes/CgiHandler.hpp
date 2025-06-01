#pragma once

#include "HttpRequest.hpp"
#include "HttpRequestParser.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "types.hpp"
#include "Utils.hpp"
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <errno.h>

class HttpRequest;
class HttpResponse; 

class CgiHandler {
    public:
        CgiHandler(HttpResponse &httpResponse, HttpRequest &httpRequest);
        ~CgiHandler();
        CgiHandler(const CgiHandler &other);
        CgiHandler &operator=(const CgiHandler &other);
        bool is_cgi_request(const std::string &path, const std::vector<std::string>& cgi_extensions);
        bool is_location_has_cgi(ConfigMap best_match_config);
        void handle_cgi_request(const std::string &cgi_path, std::vector<char> body_data, std::string method, std::string path);
        bool is_cgi_like_path(const std::string& path);
    private:
        HttpResponse &response;
        HttpRequest &request;
};