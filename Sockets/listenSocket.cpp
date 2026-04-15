#include "listenSocket.hpp"

#include <arpa/inet.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

HTTP::ListenSocket::ListenSocket(int domain, int service, int protocol, int port, u_long interface, int bklg)
    : backlog(bklg) {
    address.sin_family = domain;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(interface);

    sock = socket(domain, service, protocol);
    if (sock < 0) {
        throw std::runtime_error("socket failed");
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("setsockopt failed");
    }

    if (bind(sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("bind failed");
    }

    if (listen(sock, backlog) < 0) {
        throw std::runtime_error("listen failed");
    }
}

HTTP::ListenSocket::~ListenSocket() {
    if (sock >= 0) {
        close(sock);
    }
}

struct sockaddr_in HTTP::ListenSocket::get_address() const { return address; }

int HTTP::ListenSocket::get_sock() const { return sock; }

int HTTP::ListenSocket::get_backlog() const { return backlog; }
