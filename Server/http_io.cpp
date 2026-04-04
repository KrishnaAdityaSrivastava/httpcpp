#include "http_io.hpp"

#include <fcntl.h>
#include <iostream>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

bool ends_with(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::string get_mime_type(const std::string& path) {
    if (ends_with(path, ".html")) return "text/html";
    if (ends_with(path, ".css")) return "text/css";
    if (ends_with(path, ".js")) return "application/javascript";
    if (ends_with(path, ".png")) return "image/png";
    if (ends_with(path, ".jpg") || ends_with(path, ".jpeg")) return "image/jpeg";
    if (ends_with(path, ".gif")) return "image/gif";
    if (ends_with(path, ".json")) return "application/json";
    return "application/octet-stream";
}

std::string status_text_from_code(int status) {
    if (status == 200) return "OK";
    if (status == 201) return "Created";
    if (status == 404) return "Not Found";
    return "OK";
}

} // namespace

bool HTTP::HttpIO::read_request_from_socket(int client_socket, HTTP::Request& req) {
    std::string buffer;
    char temp[4096];

    size_t header_end = std::string::npos;
    size_t content_length = 0;

    while (true) {
        const ssize_t bytes = read(client_socket, temp, sizeof(temp));
        if (bytes == 0) {
            return false;
        }
        if (bytes < 0) {
            perror("read");
            return false;
        }

        buffer.append(temp, bytes);

        if (header_end == std::string::npos) {
            header_end = buffer.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                continue;
            }

            const std::string headers = buffer.substr(0, header_end);
            const size_t cl_pos = headers.find("Content-Length:");
            if (cl_pos != std::string::npos) {
                size_t start = cl_pos + 15;
                while (start < headers.size() && headers[start] == ' ') {
                    start++;
                }
                const size_t end = headers.find("\r\n", start);
                content_length = std::stoul(headers.substr(start, end - start));
            }
        }

        const size_t total_needed = header_end + 4 + content_length;
        if (buffer.size() < total_needed) {
            continue;
        }

        const std::string raw_request = buffer.substr(0, total_needed);
        const size_t line_end = raw_request.find("\r\n");
        const std::string request_line = raw_request.substr(0, line_end);

        const size_t first_space = request_line.find(' ');
        const size_t second_space = request_line.find(' ', first_space + 1);

        req.route.method = request_line.substr(0, first_space);
        req.route.path = request_line.substr(first_space + 1, second_space - first_space - 1);
        req.version = request_line.substr(second_space + 1);
        req.body = raw_request.substr(header_end + 4, content_length);

        std::cout << "Method: " << req.route.method << "\n";
        std::cout << "Path: " << req.route.path << "\n";
        std::cout << "Body size: " << req.body.size() << "\n";

        return true;
    }
}

void HTTP::HttpIO::send_response(int client_socket, const HTTP::Response& res) {
    std::string response;
    response.reserve(res.body.size() + 256);

    response += "HTTP/1.1 ";
    response += std::to_string(res.status);
    response += " ";
    response += status_text_from_code(res.status);
    response += "\r\n";

    bool has_length = false;
    bool has_type = false;
    bool has_conn = false;

    for (const auto& [key, value] : res.headers) {
        if (key == "Content-Length") has_length = true;
        if (key == "Content-Type") has_type = true;
        if (key == "Connection") has_conn = true;

        response += key;
        response += ": ";
        response += value;
        response += "\r\n";
    }

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

    response += "\r\n";
    response += res.body;

    size_t total_sent = 0;
    while (total_sent < response.size()) {
        const ssize_t sent = write(client_socket, response.c_str() + total_sent, response.size() - total_sent);
        if (sent <= 0) {
            break;
        }
        total_sent += static_cast<size_t>(sent);
    }
}

void HTTP::HttpIO::send_file_response(int client_socket, const std::string& path) {
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        const std::string not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(client_socket, not_found.c_str(), not_found.size());
        return;
    }

    struct stat st;
    fstat(fd, &st);

    const std::string headers = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: " +
                                get_mime_type(path) + "\r\n"
                                                      "Content-Length: " +
                                std::to_string(st.st_size) + "\r\n"
                                                           "Connection: close\r\n\r\n";

    write(client_socket, headers.c_str(), headers.size());

    off_t offset = 0;
    while (offset < st.st_size) {
        const ssize_t sent = sendfile(client_socket, fd, &offset, st.st_size - offset);
        if (sent <= 0) {
            break;
        }
    }

    close(fd);
}
