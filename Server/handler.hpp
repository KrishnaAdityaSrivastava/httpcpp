#pragma once

#include <string>
#include <unordered_map>

#include "request.hpp"
#include "response.hpp"

namespace HTTP {

class UserStore {
  private:
    std::string file_path;

  public:
    explicit UserStore(std::string path);

    bool user_exists(const std::string& username) const;
    bool register_user(const std::string& username, const std::string& password) const;
    bool check_credentials(const std::string& username, const std::string& password) const;
};

class Handler {
  public:
    virtual ~Handler() = default;
    virtual Response handle(Request& req) = 0;
};

class LoginHandler : public Handler {
  private:
    const UserStore& users;

  public:
    explicit LoginHandler(const UserStore& user_store);
    Response handle(Request& req) override;
};

class RegisterHandler : public Handler {
  private:
    const UserStore& users;

  public:
    explicit RegisterHandler(const UserStore& user_store);
    Response handle(Request& req) override;
};

class HomeHandler : public Handler {
  public:
    HomeHandler() = default;
    Response handle(Request& req) override;
};

class FileHandler : public Handler {
  private:
    std::string root;

  public:
    explicit FileHandler(std::string root_directory);
    Response handle(Request& req) override;
};

std::unordered_map<std::string, std::string> parse_form_body(const std::string& body);
std::string render_auth_page(const std::string& message = "");

} // namespace HTTP
