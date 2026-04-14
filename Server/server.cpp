#include "server.hpp"

#include <unistd.h>

#include "http_io.hpp"
#include "router.hpp"

// Creates and initializes the listening socket.
HTTP::Server::Server(int domain, int service, int protocol, int port, u_long interface, int bklg) {
    socket = new ListenSocket(domain, service, protocol, port, interface, bklg);
}

// Releases socket resources owned by the server.
HTTP::Server::~Server() {
    delete socket;
}

// Returns the listening socket object.
HTTP::ListenSocket* HTTP::Server::get_socket() { return socket; }

// Registers a GET route and its function handler.
void HTTP::Server::get(const std::string& path, HandlerFn handler) {
    routes.push_back(Route{"GET", path, handler});
}

// Registers a POST route and its function handler.
void HTTP::Server::post(const std::string& path, HandlerFn handler) {
    routes.push_back(Route{"POST", path, handler});
}

// Accepts a new client connection from the listening socket.
int HTTP::Server::accepter() {
    struct sockaddr_in address = get_socket()->get_address();
    int addrlen = sizeof(address);
    return accept(get_socket()->get_sock(), (struct sockaddr*)&address, (socklen_t*)&addrlen);
}

// Reads requests from one client and writes responses until close/timeout.
void HTTP::Server::handle_client_connection(int client_socket) {
    struct timeval tv{5, 0};
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    HTTP::HttpIO io;

    while(true){
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
        else io.send_response(client_socket, res);

        auto it = req.headers.find("Connection");
        if (it != req.headers.end() && it->second == "close") {
            break;
        }
    }
    close(client_socket);
}

// Main server loop: accept one client and handle it synchronously.
void HTTP::Server::launch() {
    while (true) {
        int client_socket = accepter();

        if (client_socket < 0) {
            continue;
        }

        handle_client_connection(client_socket);
    }
}
