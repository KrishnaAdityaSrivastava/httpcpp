#ifndef Server_h
#define Server_h

#include <string>
#include <vector>
#include <memory>

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
    Server(int domain, int service, int protocol, int port, u_long interface, int bklg);
    ~Server();

    ListenSocket* get_socket();
    void handle_client_connection(int client_socket);

    void get(const std::string& path, std::shared_ptr<IRequestHandler> handler);
    void post(const std::string& path, std::shared_ptr<IRequestHandler> handler);

    void launch();
};
} // namespace HTTP

#endif
