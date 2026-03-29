#ifndef ListenSocket_h
#define ListenSocket_h

#include <stdio.h>

#include "bindSocket.hpp"

namespace HTTP{
    class ListenSocket: public BindSocket{
        private:
            int backlog;
            int listening;
        public:
            ListenSocket(int domain, int service,int protocol, int port, u_long interface,int bklg);
            void start_listening();
    };
}

#endif