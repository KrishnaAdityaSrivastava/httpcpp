#include "server.hpp"

#include <stdexcept>
#include <unistd.h>

#include "http_io.hpp"
#include "router.hpp"

HTTP::Server::Server(int domain, int service, int protocol, int port, u_long interface, int bklg)
    : socket(new ListenSocket(domain, service, protocol, port, interface, bklg)), routes() {}

HTTP::Server::~Server() {
    delete socket;
}

HTTP::ListenSocket* HTTP::Server::get_socket() { return socket; }

void HTTP::Server::get(const std::string& path, Handler* handler) {
    routes.emplace_back("GET", path, handler);
}

void HTTP::Server::post(const std::string& path, Handler* handler) {
    routes.emplace_back("POST", path, handler);
}

int HTTP::Server::accepter() {
    struct sockaddr_in address = get_socket()->get_address();
    int addrlen = sizeof(address);
    return accept(get_socket()->get_sock(), (struct sockaddr*)&address, (socklen_t*)&addrlen);
}

void HTTP::Server::handle_client_connection(int client_socket) {
    struct timeval tv{5, 0};
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    HTTP::HttpIO io;

    while (true) {
        Request req;
        if (!io.read_request_from_socket(client_socket, req)) {
            break;
        }
        HTTP::Response res = HTTP::Router::dispatch(routes, req);

        if (res.is_file) {
            io.send_file_response(client_socket, res.file_path);
            close(client_socket);
            return;
        }
        io.send_response(client_socket, res);

        auto it = req.headers.find("Connection");
        if (it != req.headers.end() && it->second == "close") {
            break;
        }
    }
    close(client_socket);
}

void HTTP::Server::launch() {
    while (true) {
        try {
            int client_socket = accepter();
            if (client_socket < 0) {
                continue;
            }
            handle_client_connection(client_socket);
        } catch (const std::runtime_error&) {
            continue;
        }
    }
}
