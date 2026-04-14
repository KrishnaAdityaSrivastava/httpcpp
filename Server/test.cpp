#include <stdio.h>

#include <memory>
#include "server.hpp"
#include "request.hpp"
#include "response.hpp"
#include "handler.hpp"

#include <signal.h>

class HomePageHandler : public HTTP::IRequestHandler {
  public:
    HTTP::Response handle(const HTTP::Request&) const override {
        HTTP::Response res(
            200,
            "<!doctype html><html><head><title>OOP HTTP</title></head>"
            "<body><h1>OOP demo server</h1><p>This route uses polymorphism.</p></body></html>");
        res.headers["Content-Type"] = "text/html";
        return res;
    }
};

class FilePageHandler : public HTTP::IRequestHandler {
  private:
    std::string file_path;

  public:
    explicit FilePageHandler(std::string path) : file_path(std::move(path)) {}

    HTTP::Response handle(const HTTP::Request&) const override {
        HTTP::Response res;
        res.is_file = true;
        res.file_path = file_path;
        return res;
    }
};

int main(){
    signal(SIGPIPE, SIG_IGN);

    HTTP::Server server(AF_INET,SOCK_STREAM,0,8080,INADDR_ANY,16);

    server.get("/", std::make_shared<HomePageHandler>());
    server.get("/file", std::make_shared<FilePageHandler>("sample.html"));
    server.get("/about", std::make_shared<HTTP::LambdaHandler>([](const HTTP::Request&) {
        HTTP::Response res(200, "About: lambda handler example");
        res.headers["Content-Type"] = "text/plain";
        return res;
    }));

    server.launch();
}
