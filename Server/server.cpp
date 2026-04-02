#include "server.hpp"
#include <thread>

HTTP::Server::Server(int domain = AF_INET, int service = SOCK_STREAM, int protocol = 0, int port = 8080, u_long interface = INADDR_ANY, int bklg = 16){
    socket = new ListenSocket(domain,service,protocol,port,interface,bklg);
}

HTTP::ListenSocket* HTTP::Server::get_socket(){ return socket; }

// HTTP Routes
void HTTP::Server::get(const std::string& path, std::function<std::string(const Request&)> handler)
{
    routes.push_back(Route{"GET", path, handler});
}

void HTTP::Server::post(const std::string& path,std::function<std::string(const Request&)> handler)
{
    routes.push_back(Route{"POST", path, handler});
}

//accept request

int HTTP::Server::accepter()
{
    struct sockaddr_in address = get_socket()->get_address();

    int addrlen = sizeof(address);

    int client_socket = accept( get_socket()->get_sock(), (struct sockaddr *)&address, (socklen_t *)&addrlen);

    return client_socket;
}

//handle request 

// ~~ optimise later to use pointers instead 

void HTTP::Server::handler(int client_socket)
{
    //std::cout << std::this_thread::get_id() << std::endl;
    char buffer[30001];
    ssize_t bytes = read(client_socket, buffer, 30000);
    if (bytes <= 0) return;
    buffer[bytes] = '\0';

    std::string req(buffer);

    // first line = "POST /users HTTP/1.1"
    size_t pos = req.find("\r\n");
    std::string request_line = req.substr(0, pos);

    size_t first_space = request_line.find(' ');
    size_t second_space = request_line.find(' ', first_space + 1);

    std::string method = request_line.substr(0, first_space);
    std::string path = request_line.substr(first_space + 1, second_space - first_space - 1);
    std::string version = request_line.substr(second_space + 1);

    size_t body_start = req.find("\r\n\r\n");
    std::string body = "";

    if (body_start != std::string::npos) {
        body = req.substr(body_start + 4);
    }

    std::cout << "Method: " << method << "\n";
    std::cout << "Path: " << path << "\n";
    std::cout << "Body: " << body << "\n";

    Request reqObj;
    reqObj.route.method = method;
    reqObj.route.path = path;
    reqObj.version = version;
    reqObj.body = body;
    responder(client_socket,reqObj);

   cache.inc("reqcnt",1);
}
void HTTP::Server::set_handler(std::function<std::string(Request)> operation)
{
    custom_handler = operation;
}


//respond to request

void HTTP::Server::responder(int client_socket,Request& req){
    std::cout <<"Request Served by now : "<< cache.get("reqcnt");
    std::string res = "404 Not Found";

    for (auto& route : routes) {
        if (route.method != req.route.method) continue;

        std::unordered_map<std::string, std::string> params;

        if (match_path(route.path, req.route.path, params)) {
            req.params = params;
            res = route.handler(req);
            break;
        }
    }

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " 
        + std::to_string(res.size()) + "\r\n"
        "\r\n" + res;
    write(client_socket, response.c_str(), response.size());
    close(client_socket);
}

void HTTP::Server::launch()
{
    while (true)
    {
        std::cout << std::endl << "WAITING"<<std::endl;
        int client_socket = accepter();
        std::thread([this,client_socket]() {
            handler(client_socket);
        }).detach();

        std::cout <<  std::endl<<"DONE"<<std::endl;
    }
}

// UTILS
HTTP::Cache& HTTP::Server::get_cache(){ return cache; }

std::string HTTP::Server::make_key(const std::string& method, const std::string& path)
{
    return method + ":" + path;
}

bool HTTP::Server::match_path(const std::string& route_path,const std::string& req_path,std::unordered_map<std::string, std::string>& params)
{
    size_t i = 0, j = 0;
    size_t n = route_path.size();
    size_t m = req_path.size();

    while (i < n && j < m){
        // skip leading '/'
        while (i < n && route_path[i] == '/') i++;
        while (j < m && req_path[j] == '/') j++;

        if (i >= n && j >= m) return true;

        // extract next segment from route_path
        size_t i_start = i;
        while (i < n && route_path[i] != '/') i++;
        std::string route_part = route_path.substr(i_start, i - i_start);

        // extract next segment from req_path
        size_t j_start = j;
        while (j < m && req_path[j] != '/') j++;
        std::string req_part = req_path.substr(j_start, j - j_start);

        // compare
        if (!route_part.empty() && route_part[0] == ':') {
            params[route_part.substr(1)] = req_part;
        }
        else if (route_part != req_part) {
            return false;
        }
    }

    // ensure both paths fully consumed
    return i >= n && j >= m;
}
