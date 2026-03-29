#include <stdio.h>

#include "bindSocket.hpp"

HTTP::BindSocket::BindSocket(int domain, int service,int protocol, int port, u_long interface) : Socket(domain,service,protocol,port,interface){
    set_connection(connect_network(get_sock(),get_address()));
    test_connection(get_connection());

}
int HTTP::BindSocket::connect_network(int sock, struct sockaddr_in address){
    return bind(sock,(struct sockaddr *)& address,sizeof(address));
}