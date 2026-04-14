#pragma once

#include <string>
#include <cctype>
#include <fcntl.h>
#include <iostream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>


#include "request.hpp"
#include "response.hpp"

namespace HTTP{
    class HttpIO {
        public:
            //std::string buffer;
            bool read_request_from_socket(int client_socket, Request& req);
            void send_response(int client_socket, const Response& res);
            void send_file_response(int client_socket, const std::string& path);
    };
} // namespace HTTP::HttpIO
