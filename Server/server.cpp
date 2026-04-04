#include "server.hpp"

#include <iostream>
#include <thread>
#include <unistd.h>

#include "http_io.hpp"
#include "router.hpp"

HTTP::Server::Server(int domain, int service, int protocol, int port, u_long interface, int bklg) {
    socket = new ListenSocket(domain, service, protocol, port, interface, bklg);
}

HTTP::ListenSocket* HTTP::Server::get_socket() { return socket; }

void HTTP::Server::get(const std::string& path,
                       std::function<HTTP::Response(const Request&)> handler) {
    routes.push_back(Route{"GET", path, handler});
}

void HTTP::Server::post(const std::string& path,
                        std::function<HTTP::Response(const Request&)> handler) {
    routes.push_back(Route{"POST", path, handler});
}

int HTTP::Server::accepter() {
    struct sockaddr_in address = get_socket()->get_address();
    int addrlen = sizeof(address);
    return accept(get_socket()->get_sock(), (struct sockaddr*)&address, (socklen_t*)&addrlen);
}

void HTTP::Server::handle_client_connection(int client_socket) {
    Request req;
    if (!HTTP::HttpIO::read_request_from_socket(client_socket, req)) {
        close(client_socket);
        return;
    }

    process_request(client_socket, req);
    cache.inc("reqcnt", 1);
}

// void HTTP::Server::set_handler(std::function<std::string(Request)> operation) {
//     custom_handler = operation;
// }

void HTTP::Server::process_request(int client_socket, Request& req) {
    std::cout << std::endl << "Request Served by now : " << cache.get("reqcnt") << std::endl;

    HTTP::Response res = HTTP::Router::dispatch(routes, req);

    if (res.is_file) {
        HTTP::HttpIO::send_file_response(client_socket, res.file_path);
        close(client_socket);
        return;
    }

    HTTP::HttpIO::send_response(client_socket, res);
    close(client_socket);
}

void HTTP::Server::launch() {
    while (true) {
        std::cout << std::endl << "WAITING" << std::endl;

        int client_socket = accepter();
        if (client_socket < 0) {
            close(client_socket);
            return;
        }

        std::thread([this, client_socket]() { handle_client_connection(client_socket); }).detach();

        std::cout << std::endl << "DONE" << std::endl;
    }
}

HTTP::Cache& HTTP::Server::get_cache() { return cache; }

std::string HTTP::Server::make_key(const std::string& method, const std::string& path) {
    return method + ":" + path;
}
