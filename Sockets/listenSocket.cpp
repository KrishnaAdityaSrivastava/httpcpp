#include "listenSocket.hpp"

#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>

HTTP::ListenSocket::ListenSocket(int domain, int service, int protocol, int port, u_long interface, int bklg)
    : backlog(bklg) {
    address.sin_family = domain;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(interface);

    sock = socket(domain, service, protocol);
    if (sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    if (bind(sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, backlog) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
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
