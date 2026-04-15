#pragma once

#include <string>

namespace HTTP {

class Handler;

struct Route {
    std::string method;
    std::string path;
    Handler* handler;

    Route(std::string method, std::string path, Handler* handler)
        : method(std::move(method)), path(std::move(path)), handler(handler) {}
};

} // namespace HTTP
