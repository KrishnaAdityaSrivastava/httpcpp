#ifndef Server_h
#define Server_h

#include <string>
#include <vector>

#include "request.hpp"
#include "response.hpp"
#include "route.hpp"
#include "handler.hpp"

#include "../network.hpp"

namespace HTTP {
class Server {
  private:
    ListenSocket* socket;

    int accepter();
    std::vector<Route> routes;

  public:
    // Builds server with socket config and listen backlog.
    Server(int domain, int service, int protocol, int port, u_long interface, int bklg);
    // Cleans up server-owned resources.
    ~Server();

    // Exposes listening socket object.
    ListenSocket* get_socket();
    // Processes all requests from one connected client.
    void handle_client_connection(int client_socket);

    // Registers GET route handler.
    void get(const std::string& path, HandlerFn handler);
    // Registers POST route handler.
    void post(const std::string& path, HandlerFn handler);

    // Runs infinite accept/serve loop.
    void launch();
};
} // namespace HTTP

#endif
