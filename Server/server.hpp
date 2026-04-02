#ifndef Server_h
#define Server_h

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <functional>
#include <vector>
#include <unordered_map>

#include "request.hpp"
#include "route.hpp"

#include "../Cache/cache.hpp"

#include "../network.hpp"

namespace HTTP {
    class Server{
    private:
        ListenSocket* socket;
        int accepter();
        void handler(int client_socket);
        void responder(int client_socket,Request& req);

        char buffer[30000] = {0};    
        int new_socket;

        std::function<std::string(Request)> custom_handler;
        //std::unordered_map<std::string, std::function<std::string(const Request&)>> routes;
        std::vector<Route> routes;
        HTTP::Cache cache;

    public:
        Server(int domain, int service, int protocol, int port, u_long interface, int bklg);
        ListenSocket* get_socket();

        void set_handler(std::function<std::string(Request)> operation);

        std::string make_key(const std::string& method, const std::string& path);

        void get(const std::string& path, std::function<std::string(const Request&)> handler);
        void post(const std::string& path,std::function<std::string(const Request&)> handler);

        bool match_path(const std::string& route_path, const std::string& req_path,std::unordered_map<std::string, std::string>& params);

        Cache& get_cache();

        void launch();
        
    };   
}

#endif