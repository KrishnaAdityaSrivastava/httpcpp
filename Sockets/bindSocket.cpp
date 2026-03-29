#include "bindSocket.hpp"

HTTP::BindSocket::BindSocket(int domain, int service, int protocol, int port, u_long interface)
    : Socket(domain, service, protocol, port, interface) {
    establish_connection();
}

int HTTP::BindSocket::connect_network(int sock, struct sockaddr_in address) {
    return bind(sock, (struct sockaddr *)&address, sizeof(address));
}
