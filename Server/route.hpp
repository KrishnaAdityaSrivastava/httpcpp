#pragma once
#include <string>
#include <functional>

namespace HTTP {
struct Request;
struct Route {
    std::string method;
    std::string path;
    std::function<std::string(const Request&)> handler;
};

}