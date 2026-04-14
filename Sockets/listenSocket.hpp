#ifndef ListenSocket_h
#define ListenSocket_h

#include <netinet/in.h>

namespace HTTP {
class ListenSocket {
  private:
    struct sockaddr_in address{};
    int sock{-1};
    int backlog;

  public:
    ListenSocket(int domain, int service, int protocol, int port, u_long interface, int bklg);
    ~ListenSocket();

    struct sockaddr_in get_address() const;
    int get_sock() const;
    int get_backlog() const;
};
} // namespace HTTP

#endif
