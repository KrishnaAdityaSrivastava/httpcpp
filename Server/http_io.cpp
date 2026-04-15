#include "http_io.hpp"

#include <stdexcept>
#include <string>

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
    if (code == 500) return "Internal Server Error";
    return "OK";
}

} // namespace

bool HTTP::HttpIO::read_request_from_socket(int client_socket, HTTP::Request& req) {
    std::string buffer;
    char temp[4096];

    int bytes = read(client_socket, temp, sizeof(temp));
    if (bytes <= 0) return false;

    buffer = std::string(temp, bytes);

    size_t line_end = buffer.find("\r\n");
    if (line_end == std::string::npos) {
        throw std::runtime_error("Malformed request line");
    }

    const std::string request_line = buffer.substr(0, line_end);
    size_t sp1 = request_line.find(' ');
    size_t sp2 = request_line.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        throw std::runtime_error("Malformed request line");
    }

    req.method = request_line.substr(0, sp1);
    req.path = request_line.substr(sp1 + 1, sp2 - sp1 - 1);
    req.version = request_line.substr(sp2 + 1);

    size_t header_start = line_end + 2;
    size_t body_pos = buffer.find("\r\n\r\n", header_start);
    size_t header_end = body_pos == std::string::npos ? buffer.size() : body_pos;

    while (header_start < header_end) {
        size_t header_line_end = buffer.find("\r\n", header_start);
        if (header_line_end == std::string::npos || header_line_end > header_end) {
            break;
        }
        const std::string header_line = buffer.substr(header_start, header_line_end - header_start);
        size_t colon = header_line.find(':');
        if (colon != std::string::npos) {
            std::string key = header_line.substr(0, colon);
            std::string value = header_line.substr(colon + 1);
            while (!value.empty() && value[0] == ' ') {
                value.erase(0, 1);
            }
            req.headers[key] = value;
        }
        header_start = header_line_end + 2;
    }

    req.body.clear();
    if (body_pos != std::string::npos && body_pos + 4 <= buffer.size()) {
        req.body = buffer.substr(body_pos + 4);
    }

    return true;
}

void HTTP::HttpIO::send_response(int client_socket, const HTTP::Response& res) {
    std::string content_type = "text/html";
    const auto header_it = res.headers.find("Content-Type");
    if (header_it != res.headers.end()) {
        content_type = header_it->second;
    }

    std::string response =
        "HTTP/1.1 " + std::to_string(res.status) + " " + status_text(res.status) + "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(res.body.size()) + "\r\n"
        "\r\n" +
        res.body;

    write(client_socket, response.c_str(), response.size());
}

void HTTP::HttpIO::send_file_response(int client_socket, const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        const std::string msg = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(client_socket, msg.c_str(), msg.size());
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        throw std::runtime_error("Failed to read file metadata");
    }

    std::string header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: " + get_mime_type(path) + "\r\n"
        "Content-Length: " + std::to_string(st.st_size) + "\r\n"
        "\r\n";

    write(client_socket, header.c_str(), header.size());

    char chunk[4096];
    while (true) {
        int read_bytes = read(fd, chunk, sizeof(chunk));
        if (read_bytes <= 0) {
            break;
        }
        write(client_socket, chunk, read_bytes);
    }

    close(fd);
}
