#include <csignal>
#include <iostream>
#include <stdexcept>

#include "handler.hpp"
#include "server.hpp"

int main() {
    signal(SIGPIPE, SIG_IGN);

    try {
        HTTP::Server server(AF_INET, SOCK_STREAM, 0, 8080, INADDR_ANY, 16);

        HTTP::UserStore store("users.txt");
        HTTP::HomeHandler home_handler;
        HTTP::RegisterHandler register_handler(store);
        HTTP::LoginHandler login_handler(store);
        HTTP::FileHandler file_handler("public");

        server.get("/", &home_handler);
        server.post("/register", &register_handler);
        server.post("/login", &login_handler);
        server.get("/files/:name", &file_handler);

        server.launch();
    } catch (const std::runtime_error& ex) {
        std::cerr << "Server failed: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
