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
    // Creates and starts listening socket.
    ListenSocket(int domain, int service, int protocol, int port, u_long interface, int bklg);
    // Closes socket descriptor.
    ~ListenSocket();

    // Returns bound address.
    struct sockaddr_in get_address() const;
    // Returns socket descriptor.
    int get_sock() const;
    // Returns backlog value.
    int get_backlog() const;
};
} // namespace HTTP

#endif
