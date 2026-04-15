#include <stdio.h>

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "handler.hpp"
#include "request.hpp"
#include "response.hpp"
#include "server.hpp"

#include <signal.h>

namespace {

const std::string kUserStorePath = "users.txt";

std::string html_page(const std::string& content) {
    return "<!doctype html><html><head><meta charset=\"utf-8\"><title>Auth Test</title>"
           "<style>body{font-family:sans-serif;max-width:700px;margin:40px auto;}"
           "form{padding:16px;border:1px solid #ccc;border-radius:8px;margin-bottom:18px;}"
           "label{display:block;margin:8px 0 4px;}"
           "input{padding:8px;width:100%;max-width:320px;}"
           "button{margin-top:10px;padding:8px 12px;}"
           "pre{background:#f7f7f7;padding:8px;border-radius:4px;}</style></head><body>" +
           content + "</body></html>";
}

int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

std::string url_decode(const std::string& encoded) {
    std::string decoded;
    decoded.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '+') {
            decoded.push_back(' ');
            continue;
        }
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int hi = hex_to_int(encoded[i + 1]);
            int lo = hex_to_int(encoded[i + 2]);
            if (hi >= 0 && lo >= 0) {
                decoded.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                continue;
            }
        }
        decoded.push_back(encoded[i]);
    }

    return decoded;
}

std::unordered_map<std::string, std::string> parse_form_body(const std::string& body) {
    std::unordered_map<std::string, std::string> form;
    size_t start = 0;

    while (start < body.size()) {
        size_t end = body.find('&', start);
        if (end == std::string::npos) end = body.size();

        size_t eq = body.find('=', start);
        if (eq != std::string::npos && eq < end) {
            std::string key = url_decode(body.substr(start, eq - start));
            std::string value = url_decode(body.substr(eq + 1, end - eq - 1));
            form[key] = value;
        }

        start = end + 1;
    }

    return form;
}

bool register_user(const std::string& username, const std::string& password) {
    std::ifstream existing(kUserStorePath);
    std::string line;
    while (std::getline(existing, line)) {
        std::stringstream ss(line);
        std::string saved_user;
        std::getline(ss, saved_user, ':');
        if (saved_user == username) {
            return false;
        }
    }

    std::ofstream out(kUserStorePath, std::ios::app);
    if (!out.is_open()) {
        return false;
    }

    out << username << ':' << password << '\n';
    return true;
}

bool check_user_credentials(const std::string& username, const std::string& password) {
    std::ifstream in(kUserStorePath);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        std::stringstream ss(line);
        std::string saved_user;
        std::string saved_pass;
        std::getline(ss, saved_user, ':');
        std::getline(ss, saved_pass);

        if (saved_user == username && saved_pass == password) {
            return true;
        }
    }

    return false;
}

HTTP::Response render_login_page(const std::string& message = "") {
    std::string content = "<h1>Auth demo using current HTTP framework</h1>";

    if (!message.empty()) {
        content += "<p><strong>Status:</strong> " + message + "</p>";
    }

    content +=
        "<form method=\"post\" action=\"/register\">"
        "<h2>Create account</h2>"
        "<label for=\"reg-username\">Username</label>"
        "<input id=\"reg-username\" name=\"username\" required>"
        "<label for=\"reg-password\">Password</label>"
        "<input id=\"reg-password\" name=\"password\" type=\"password\" required>"
        "<button type=\"submit\">Register</button>"
        "</form>"

        "<form method=\"post\" action=\"/login\">"
        "<h2>Login</h2>"
        "<label for=\"login-username\">Username</label>"
        "<input id=\"login-username\" name=\"username\" required>"
        "<label for=\"login-password\">Password</label>"
        "<input id=\"login-password\" name=\"password\" type=\"password\" required>"
        "<button type=\"submit\">Login</button>"
        "</form>"

        "<p>Users are stored in <code>users.txt</code> as <code>username:password</code>.</p>";

    HTTP::Response res(200, html_page(content));
    res.headers["Content-Type"] = "text/html";
    return res;
}

} // namespace

// Handler for "/" route: returns the login/register page.
HTTP::Response home_page(const HTTP::Request&) { return render_login_page(); }

HTTP::Response register_page(const HTTP::Request& req) {
    auto form = parse_form_body(req.body);

    const std::string username = form["username"];
    const std::string password = form["password"];

    if (username.empty() || password.empty()) {
        return render_login_page("Registration failed: username and password are required.");
    }

    if (!register_user(username, password)) {
        return render_login_page("Registration failed: username exists or file error.");
    }

    return render_login_page("Registration successful for user: " + username);
}

HTTP::Response login_page(const HTTP::Request& req) {
    auto form = parse_form_body(req.body);

    const std::string username = form["username"];
    const std::string password = form["password"];

    if (username.empty() || password.empty()) {
        return render_login_page("Login failed: username and password are required.");
    }

    if (check_user_credentials(username, password)) {
        return render_login_page("Login successful. Welcome " + username + "!");
    }

    return render_login_page("Login failed: invalid username or password.");
}

// Entry point: registers routes and starts the HTTP server loop.
int main() {
    signal(SIGPIPE, SIG_IGN);

    HTTP::Server server(AF_INET, SOCK_STREAM, 0, 8080, INADDR_ANY, 16);

    server.get("/", home_page);
    server.post("/register", register_page);
    server.post("/login", login_page);

    server.launch();
}
