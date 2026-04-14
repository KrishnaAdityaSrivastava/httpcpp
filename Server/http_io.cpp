#include "http_io.hpp"
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace {

    // Returns true if str ends with suffix.
    bool ends_with(const std::string& str, const std::string& suffix) {
        if (str.length() < suffix.length()) {
            return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    // Maps file extension to MIME type for HTTP Content-Type header.
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

    // Converts common HTTP status code to status text.
    std::string status_text_from_code(int status) {
        if (status == 200) return "OK";
        if (status == 201) return "Created";
        if (status == 404) return "Not Found";
        return "OK";
    }

    constexpr size_t MAX_HEADER_BYTES = 16 * 1024;       // 16 KB
    constexpr size_t MAX_BODY_BYTES   = 1 * 1024 * 1024; // 1 MB

    // Parses a decimal positive integer into size_t safely.
    bool parse_size_t_decimal(std::string_view s, size_t& out) {
        if (s.empty()) return false;
        unsigned long long tmp = 0;
        auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), tmp);
        if (ec != std::errc{} || ptr != s.data() + s.size()) return false;
        out = static_cast<size_t>(tmp);
        return true;
    }

} // namespace

// Reads one HTTP request from the socket and fills Request fields.
bool HTTP::HttpIO::read_request_from_socket(int client_socket, HTTP::Request& req) {
    std::string buffer;
    buffer.reserve(4096);
    char temp[4096];

    size_t header_end = std::string::npos;
    size_t content_length = 0;

    while (true) {
        const ssize_t bytes = ::read(client_socket, temp, sizeof(temp));
        if (bytes == 0) return false;   // peer closed
        if (bytes < 0) {
            perror("read");
            return false;
        }

        buffer.append(temp, static_cast<size_t>(bytes));

        // Header size cap before header terminator found
        if (header_end == std::string::npos && buffer.size() > MAX_HEADER_BYTES) {
            return false; // too large header
        }

        if (header_end == std::string::npos) {
            header_end = buffer.find("\r\n\r\n");
            if (header_end == std::string::npos) continue;

            // header bytes excludes trailing \r\n\r\n
            if (header_end > MAX_HEADER_BYTES) return false;

            std::string_view headers(buffer.data(), header_end);

            // Parse Content-Length safely (case-sensitive for now)
            size_t cl_pos = headers.find("Content-Length:");
            if (cl_pos != std::string::npos) {
                size_t start = cl_pos + 15;
                while (start < headers.size() && headers[start] == ' ') ++start;

                size_t end = headers.find("\r\n", start);
                if (end == std::string::npos) end = headers.size();

                std::string_view clv = headers.substr(start, end - start);
                if (!parse_size_t_decimal(clv, content_length)) return false;
                if (content_length > MAX_BODY_BYTES) return false;
            } else {
                content_length = 0;
            }
        }

        // Overflow-safe check
        if (header_end > SIZE_MAX - 4) return false;

        size_t total_needed = header_end + 4;

        if (content_length > SIZE_MAX - total_needed) return false;
        
        total_needed += content_length;

        if (buffer.size() < total_needed) continue;

        // Parse request line from buffered data
        size_t line_end = buffer.find("\r\n");
        if (line_end == std::string::npos || line_end == 0) return false;

        std::string_view request_line(buffer.data(), line_end);

        size_t first_space = request_line.find(' ');
        if (first_space == std::string::npos || first_space == 0) return false;

        size_t second_space = request_line.find(' ', first_space + 1);
        if (second_space == std::string::npos || second_space <= first_space + 1) return false;

        std::string method(request_line.substr(0, first_space));
        std::string path(request_line.substr(first_space + 1, second_space - first_space - 1));
        std::string version(request_line.substr(second_space + 1));

        // Optional protocol checks
        if (version != "HTTP/1.1" && version != "HTTP/1.0") return false;
        if (method != "GET" && method != "POST" && method != "PUT" &&
            method != "DELETE" && method != "PATCH" && method != "HEAD") {
            return false;
        }

        req.route.method = std::move(method);
        req.route.path   = std::move(path);
        req.version      = std::move(version);
        req.body.assign(buffer.data() + header_end + 4, content_length);

        //size_t consumed = total_needed;
        //buffer.erase(0, consumed);
        return true;
    }
}


// bool HTTP::HttpIO::read_request_from_socket(int client_socket, HTTP::Request& req) {
//     std::string buffer;
//     char temp[4096];

//     size_t header_end = std::string::npos;
//     size_t content_length = 0;

//     while (true) {
//         const ssize_t bytes = read(client_socket, temp, sizeof(temp));
//         if (bytes == 0) {
//             return false;
//         }
//         if (bytes < 0) {
//             perror("read");
//             return false;
//         }

//         buffer.append(temp, bytes);

//         if (header_end == std::string::npos) {
//             header_end = buffer.find("\r\n\r\n");
//             if (header_end == std::string::npos) {
//                 continue;
//             }

//             const std::string headers = buffer.substr(0, header_end);
//             const size_t cl_pos = headers.find("Content-Length:");
//             if (cl_pos != std::string::npos) {
//                 size_t start = cl_pos + 15;
//                 while (start < headers.size() && headers[start] == ' ') {
//                     start++;
//                 }
//                 const size_t end = headers.find("\r\n", start);
//                 content_length = std::stoul(headers.substr(start, end - start));
//                 //if (content_length > )
//             }
//         }

//         const size_t total_needed = header_end + 4 + content_length;
//         if (buffer.size() < total_needed) {
//             continue;
//         }

//         const std::string raw_request = buffer.substr(0, total_needed);
//         const size_t line_end = raw_request.find("\r\n");
//         const std::string request_line = raw_request.substr(0, line_end);

//         const size_t first_space = request_line.find(' ');
//         const size_t second_space = request_line.find(' ', first_space + 1);

//         req.route.method = request_line.substr(0, first_space);
//         req.route.path = request_line.substr(first_space + 1, second_space - first_space - 1);
//         req.version = request_line.substr(second_space + 1);
//         req.body = raw_request.substr(header_end + 4, content_length);

//         std::cout << "Method: " << req.route.method << "\n";
//         std::cout << "Path: " << req.route.path << "\n";
//         std::cout << "Body size: " << req.body.size() << "\n";

//         return true;
//     }
// }

// Sends a text/body HTTP response using writev for headers+body.
void HTTP::HttpIO::send_response(int client_socket, const HTTP::Response& res) {
    std::string headers;
    headers.reserve(256);

    headers += "HTTP/1.1 " + std::to_string(res.status) + " " + status_text_from_code(res.status) + "\r\n";

    // std::string response;
    // response.reserve(res.body.size() + 256);

    // response += "HTTP/1.1 ";
    // response += std::to_string(res.status);
    // response += " ";
    // response += status_text_from_code(res.status);
    // response += "\r\n";

    bool has_length = false;
    bool has_type = false;
    bool has_conn = false;

    for (const auto& [key, value] : res.headers) {
        if (key == "Content-Length") has_length = true;
        if (key == "Content-Type") has_type = true;
        if (key == "Connection") has_conn = true;

        headers += key + ": " + value + "\r\n";
    }

    if (!has_type) headers += "Content-Type: text/plain\r\n";
    if (!has_conn) {
        headers += "Connection: keep-alive\r\n";
        headers += "Keep-Alive: timeout=5, max=100\r\n";
    }
    if (!has_length) headers += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";

    headers += "\r\n";

    struct iovec iov[2];
    iov[0].iov_base = (void*)headers.data();
    iov[0].iov_len  = headers.size();
    iov[1].iov_base = (void*)res.body.data();
    iov[1].iov_len  = res.body.size();

    size_t total_sent = 0;
    while (total_sent < headers.size()+ res.body.size()) {
        ssize_t sent = writev(client_socket, iov, 2);

        if (sent <= 0) break;

        total_sent += sent;

        if (sent >= (ssize_t)iov[0].iov_len) {
            sent -= iov[0].iov_len;
            iov[0].iov_len = 0;
            iov[1].iov_base = (char*)iov[1].iov_base + sent;
            iov[1].iov_len -= sent;
        } else {
            iov[0].iov_base = (char*)iov[0].iov_base + sent;
            iov[0].iov_len -= sent;
        }
    }
}

// Sends a file response by opening path directly and streaming with sendfile.
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
