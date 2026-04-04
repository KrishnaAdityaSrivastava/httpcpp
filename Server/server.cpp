#include "server.hpp"
#include <thread>

HTTP::Server::Server(int domain = AF_INET, int service = SOCK_STREAM, int protocol = 0, int port = 8080, u_long interface = INADDR_ANY, int bklg = 16){
    socket = new ListenSocket(domain,service,protocol,port,interface,bklg);
}

HTTP::ListenSocket* HTTP::Server::get_socket(){ return socket; }

// HTTP Routes
void HTTP::Server::get(const std::string& path, std::function<HTTP::Response(const Request&)> handler)
{
    routes.push_back(Route{"GET", path, handler});
}

void HTTP::Server::post(const std::string& path,std::function<HTTP::Response(const Request&)> handler)
{
    routes.push_back(Route{"POST", path, handler});
}

void HTTP::Server::sendFile(int client_socket,std::string path){
    int fd = open(path.c_str(), O_RDONLY);

    struct stat st;
    fstat(fd, &st);

    // Send headers first
    std::string headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(st.st_size) + "\r\n"
        "Connection: close\r\n"
        "\r\n";

    write(client_socket, headers.c_str(), headers.size());

    // Then send file (zero-copy)
    off_t offset = 0;
    sendfile(client_socket, fd, &offset, st.st_size);

    close(fd);
}
// accept request

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
    std::string buffer;
    char temp[4096]; // TCP can split data so we need to accumulate

    ssize_t bytes;

    size_t header_end = std::string::npos;
    size_t content_length = 0;

    while (true)
    {
        bytes = read(client_socket, temp, sizeof(temp));
        if (bytes == 0) {
            close(client_socket);
            return;
        }
        if (bytes < 0) {
            perror("read");
            close(client_socket);
            return;
        }

        buffer.append(temp, bytes);

        // Check if headers are complete
        if (header_end == std::string::npos) {
            header_end = buffer.find("\r\n\r\n");
            if (header_end == std::string::npos) continue;

            // Parse Content-Length
            std::string headers = buffer.substr(0, header_end);

            size_t cl_pos = headers.find("Content-Length:");
            if (cl_pos != std::string::npos) {
                size_t start = cl_pos + 15;
                while (start < headers.size() && headers[start] == ' ') start++;

                size_t end = headers.find("\r\n", start);
                content_length = std::stoul(headers.substr(start, end - start));
            }
        }

        // Check if full body is received
        size_t total_needed = header_end + 4 + content_length;

        if (buffer.size() < total_needed) continue;

        std::string req = buffer.substr(0, total_needed);

        size_t pos = req.find("\r\n");
        std::string request_line = req.substr(0, pos);

        size_t first_space = request_line.find(' ');
        size_t second_space = request_line.find(' ', first_space + 1);

        std::string method = request_line.substr(0, first_space);
        std::string path = request_line.substr(first_space + 1, second_space - first_space - 1);
        std::string version = request_line.substr(second_space + 1);

        std::string body = req.substr(header_end + 4, content_length);

        // Debug
        std::cout << "Method: " << method << "\n";
        std::cout << "Path: " << path << "\n";
        std::cout << "Body size: " << body.size() << "\n";

        Request reqObj;
        reqObj.route.method = method;
        reqObj.route.path = path;
        reqObj.version = version;
        reqObj.body = body;

        responder(client_socket, reqObj);
        cache.inc("reqcnt", 1);

        return; // single request per connection
    }
}
void HTTP::Server::set_handler(std::function<std::string(Request)> operation)
{
    custom_handler = operation;
}

// respond to request

void HTTP::Server::responder(int client_socket,Request& req){
    std::cout <<std::endl<<"Request Served by now : "<< cache.get("reqcnt")<<std::endl;

    HTTP::Response res;
    res.status = 404;

    for (auto& route : routes) {
        if (route.method != req.route.method) continue;

        std::unordered_map<std::string, std::string> params;

        if (match_path(route.path, req.route.path, params)) {
            req.params = params;
            try {
                res = route.handler(req);
            } catch (...) {
                res.status = 500;
                res.body = "Internal Server Error";
            }
            break;
        }
    }

    if(res.is_file){
        sendFile(client_socket,res.file_path);
        close(client_socket);
        return;
    }

    std::string status_text = (res.status == 200) ? "OK" :
                          (res.status == 201) ? "Created" :
                          (res.status == 404) ? "Not Found" : "OK";
    //std::string res = "404 Not Found";

   //std::string status = (res == "404 Not Found") ? "404 Not Found" : "200 OK";

    std::string response;

    // Status line
    response.reserve(res.body.size() + 256); // avoid reallocations
    response += "HTTP/1.1 ";
    response += std::to_string(res.status);
    response += " ";
    response += status_text;
    response += "\r\n";

    // Track required headers
    bool has_length = false;
    bool has_type = false;
    bool has_conn = false;

    // Headers
    for (const auto& [key, value] : res.headers) {
        if (key == "Content-Length") has_length = true;
        if (key == "Content-Type") has_type = true;
        if (key == "Connection") has_conn = true;

        response += key;
        response += ": ";
        response += value;
        response += "\r\n";
    }

    // Required defaults
    if (!has_type) {
        response += "Content-Type: text/plain\r\n";
    }
    if (!has_conn) {
        response += "Connection: close\r\n";
    }
    if (!has_length) {
        response += "Content-Length: ";
        response += std::to_string(res.body.size());
        response += "\r\n";
    }

    // End headers
    response += "\r\n";

    // Body
    response += res.body;
    // Send Response
    size_t total_sent = 0;
    size_t to_send = response.size();
    const char* data = response.c_str();

    while (total_sent < to_send) {
        ssize_t sent = write(client_socket, data + total_sent, to_send - total_sent);
        if (sent <= 0) break;
        total_sent += sent;
    }
    close(client_socket);
}

void HTTP::Server::launch()
{
    while (true)
    {
        std::cout << std::endl << "WAITING"<<std::endl;
        int client_socket = accepter();
        if (client_socket < 0) {
            close(client_socket);
            return;
        }
        std::thread([this,client_socket]() {
            handler(client_socket);
        }).detach();

        std::cout <<  std::endl<<"DONE"<<std::endl;
    }
}

// UTILS
bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::string get_mime_type(const std::string& path) {
    if (ends_with(path,".html")) return "text/html";
    if (ends_with(path,".css")) return "text/css";
    if (ends_with(path,".js")) return "application/javascript";
    if (ends_with(path,".png")) return "image/png";
    if (ends_with(path,".jpg") || ends_with( path,".jpeg")) return "image/jpeg";
    if (ends_with(path,".gif")) return "image/gif";
    if (ends_with(path,".json")) return "application/json";
    return "application/octet-stream"; // default (ANY file)
}
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
