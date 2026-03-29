#include "listenSocket.hpp"

#include <iostream>

int main() {
    std::cout << "Starting" << std::endl;

    constexpr int kPort = 8080;
    constexpr int kBacklog = 16;

    HTTP::ListenSocket server(AF_INET, SOCK_STREAM, 0, kPort, INADDR_ANY, kBacklog);

    std::cout << "Listening on port " << kPort << " with backlog " << server.get_backlog() << std::endl;
    return 0;
}
