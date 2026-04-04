#pragma once

#include <unordered_map>
#include <vector>

#include "request.hpp"
#include "response.hpp"
#include "route.hpp"

namespace HTTP::Router {

bool match_path(const std::string& route_path, const std::string& req_path,
                std::unordered_map<std::string, std::string>& params);
Response dispatch(const std::vector<Route>& routes, Request& req);

} // namespace HTTP::Router
