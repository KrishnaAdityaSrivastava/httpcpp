#pragma once

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "request.hpp"
#include "response.hpp"

namespace HTTP {
class HttpIO {
  public:
    bool read_request_from_socket(int client_socket, Request& req);

    void send_response(int client_socket, const Response& res);

    void send_file_response(int client_socket, const std::string& path);
};
} // namespace HTTP
