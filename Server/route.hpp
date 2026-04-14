#pragma once
#include <string>
#include "response.hpp"

namespace HTTP {
struct Request;
using HandlerFn = Response (*)(const Request&);

struct Route {
    std::string method;
    std::string path;
    HandlerFn handler;
};

} // namespace HTTP
