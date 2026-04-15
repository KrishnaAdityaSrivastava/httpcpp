#pragma once

#include <string>
#include <unordered_map>

namespace HTTP {

struct Request {
    std::string method;
    std::string path;
    std::string version;
    std::string body;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> headers;

    Request() : method(), path(), version("HTTP/1.1"), body(), params(), headers() {}
};

} // namespace HTTP
