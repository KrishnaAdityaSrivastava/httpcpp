#include "socket.hpp"

HTTP::Socket::Socket(int domain, int service,int protocol, int port, u_long interface){
    address.sin_family = domain;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(interface);

    sock = socket( domain, service, protocol);
    test_connection(sock);

    connection = connect_network( sock, address);
    test_connection(connection);
}

void HTTP::Socket::test_connection(int test_obj){
    if (test_obj < 0){
        perror("Failed to connect...");
        exit(EXIT_FAILURE);
    }
}

struct sockaddr_in HTTP::Socket::get_address(){
    return address;
}
int HTTP::Socket::get_connection(){
    return connection;
}
int HTTP::Socket::get_sock(){
    return sock;
}

void HTTP::Socket::set_connection(int con){
    connection = con;
}