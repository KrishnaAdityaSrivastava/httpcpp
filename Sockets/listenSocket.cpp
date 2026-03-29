#include "listenSocket.hpp"

HTTP::ListenSocket::ListenSocket(int domain, int service,int protocol, int port, u_long interface,int bklg) : BindSocket(domain,service,protocol,port,interface){
    backlog = bklg;
    start_listening();
    test_connection(listening);
}

void HTTP::ListenSocket::start_listening(){
    listening = listen(get_sock(),backlog);
}

int HTTP::ListenSocket::get_listening(){
    return listening;
}

int HTTP::ListenSocket::get_backlog(){
    return backlog;
}

