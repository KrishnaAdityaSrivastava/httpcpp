#include "http_io.hpp"
#include <charconv>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

namespace {

std::string get_mime_type(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".jpg") != std::string::npos) return "image/jpeg";
    return "text/plain";
}

std::string status_text(int code) {
    if (code == 200) return "OK";
    if (code == 404) return "Not Found";
    return "OK";
}

}

bool HTTP::HttpIO::read_request_from_socket(int client_socket, HTTP::Request& req) {
    std::string buffer;
    char temp[4096];

    int bytes = read(client_socket, temp, sizeof(temp));
    if (bytes <= 0) return false;

    buffer = std::string(temp, bytes);

    size_t sp1 = buffer.find(' ');
    size_t sp2 = buffer.find(' ', sp1 + 1);
    size_t line_end = buffer.find("\r\n");

    req.route.method = buffer.substr(0, sp1);
    req.route.path   = buffer.substr(sp1 + 1, sp2 - sp1 - 1);
    req.version      = buffer.substr(sp2 + 1, line_end - sp2 - 1);

    size_t body_pos = buffer.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        req.body = buffer.substr(body_pos + 4);
    }

    return true;
}

void HTTP::HttpIO::send_response(int client_socket, const HTTP::Response& res) {
    std::string body = res.body;

    std::string response =
        "HTTP/1.1 " + std::to_string(res.status) + " " + status_text(res.status) + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "\r\n" +
        body;

    write(client_socket, response.c_str(), response.size());
}

void HTTP::HttpIO::send_file_response(int client_socket, const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        std::string msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(client_socket, msg.c_str(), msg.size());
        return;
    }

    struct stat st;
    fstat(fd, &st);

    std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + get_mime_type(path) + "\r\n"
        "Content-Length: " + std::to_string(st.st_size) + "\r\n"
        "\r\n";

    write(client_socket, header.c_str(), header.size());

    off_t offset = 0;
    sendfile(client_socket, fd, &offset, st.st_size);

    close(fd);
}