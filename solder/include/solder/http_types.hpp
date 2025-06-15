#pragma once
#include <string>
#include <unordered_map>

namespace solder {

struct Req {
    std::string method;
    std::string path;
    std::string query;
    int minor_version = 1;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    // Parse query parameters
    std::unordered_map<std::string, std::string> get_query_params() const;

    // Helper methods
    bool has_header(const std::string& name) const;
    std::string get_header(const std::string& name, const std::string& default_value = "") const;
};

struct Res {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    // Convenience methods for common responses
    static Res ok(const std::string& body = "");
    static Res created(const std::string& body = "");
    static Res no_content();
    static Res bad_request(const std::string& message = "Bad Request");
    static Res not_found(const std::string& message = "Not Found");
    static Res internal_error(const std::string& message = "Internal Server Error");

    std::string to_string() const;


void append_to(std::string& out) const;

};


}
