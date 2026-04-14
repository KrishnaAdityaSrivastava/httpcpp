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
            // Reads and parses one HTTP request from client socket.
            bool read_request_from_socket(int client_socket, Request& req);
            // Sends status line, headers, and body to client socket.
            void send_response(int client_socket, const Response& res);
            // Sends file contents from path to client socket.
            void send_file_response(int client_socket, const std::string& path);
    };
} // namespace HTTP::HttpIO
