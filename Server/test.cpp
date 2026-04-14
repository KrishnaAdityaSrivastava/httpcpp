#include <stdio.h>

#include "server.hpp"
#include "request.hpp"
#include "response.hpp"

#include <signal.h>

// Handler for "/" route: returns a basic HTML page.
HTTP::Response home_page(const HTTP::Request&) {
    HTTP::Response res(
        200,
        "<!doctype html><html><head><title>Basic C-style HTTP</title></head>"
        "<body><h1>Simple server</h1><p>Only essentials.</p></body></html>");
    res.headers["Content-Type"] = "text/html";
    return res;
}

// Handler for "/about" route: returns plain text.
HTTP::Response about_page(const HTTP::Request&) {
    HTTP::Response res(200, "About: plain function handler");
    res.headers["Content-Type"] = "text/plain";
    return res;
}

// Handler for "/file" route: marks response as file-based.
HTTP::Response file_page(const HTTP::Request&) {
    HTTP::Response res;
    res.is_file = true;
    res.file_path = "sample.html";
    return res;
}

// Entry point: registers routes and starts the HTTP server loop.
int main(){
    signal(SIGPIPE, SIG_IGN);

    HTTP::Server server(AF_INET,SOCK_STREAM,0,8080,INADDR_ANY,16);

    server.get("/", home_page);
    server.get("/about", about_page);
    server.get("/file", file_page);

    server.launch();
}
