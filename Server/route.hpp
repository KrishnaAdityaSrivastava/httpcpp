#pragma once
#include <string>
#include <memory>

#include "handler.hpp"

namespace HTTP {
struct Route {
    std::string method;
    std::string path;
    std::shared_ptr<IRequestHandler> handler;
};

} // namespace HTTP
