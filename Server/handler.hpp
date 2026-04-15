#pragma once

#include <functional>

#include "response.hpp"

namespace HTTP {
struct Request;

class IRequestHandler {
  public:
    virtual ~IRequestHandler() = default;
    virtual Response handle(const Request& req) const = 0;
};

class LambdaHandler : public IRequestHandler {
  private:
    std::function<Response(const Request&)> fn;

  public:
    explicit LambdaHandler(std::function<Response(const Request&)> handler_fn)
        : fn(std::move(handler_fn)) {}

    Response handle(const Request& req) const override { return fn(req); }
};

}
