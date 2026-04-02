#include <stdio.h>

#include "server.hpp"
#include "request.hpp"
#include "../Cache/cache.hpp"


int main(){
    

    HTTP::Server server(AF_INET,SOCK_STREAM,0,8080,INADDR_ANY,16);
    server.get("/", [](const HTTP::Request& req) {
        return "Home page";
    });

    server.get("/about", [](const HTTP::Request& req) {
        return "About page";
    });

    server.get("/users/:id", [&server](const HTTP::Request& req) {
        std::string id = req.params.at("id");
        server.get_cache().put("id",id);
        return server.get_cache().get("id");
        //return "User ID: " + cache.get(id);
    });

    server.post("/", [](const HTTP::Request& req) {
        return "Received: " + req.body;
    });

    server.launch();
}