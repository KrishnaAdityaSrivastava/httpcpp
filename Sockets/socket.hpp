#ifndef Socket_h
#define Socket_h

#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

namespace HTTP
{
    class Socket{
        private:
            struct sockaddr_in address;
            int sock;

        public:
            Socket( int domain, int service,int protocol, int port, u_long interface);
            virtual int connect_network(int sock, struct sockaddr_in address) = 0;
            void test_connection(int);

            struct sockaddr_in get_address();
            int get_sock();

            void set_connection(int domain);
    };
}
#endif