#pragma once

#include <string>

#include "request.hpp"
#include "response.hpp"

namespace HTTP::HttpIO {

bool read_request_from_socket(int client_socket, Request& req);
void send_response(int client_socket, const Response& res);
void send_file_response(int client_socket, const std::string& path);

} // namespace HTTP::HttpIO
