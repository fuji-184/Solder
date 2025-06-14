#pragma once
#include <string>
#include <unordered_map>

namespace obeng {

struct HttpRequest {
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

struct HttpResponse {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    // Convenience methods for common responses
    static HttpResponse ok(const std::string& body = "");
    static HttpResponse created(const std::string& body = "");
    static HttpResponse no_content();
    static HttpResponse bad_request(const std::string& message = "Bad Request");
    static HttpResponse not_found(const std::string& message = "Not Found");
    static HttpResponse internal_error(const std::string& message = "Internal Server Error");

    std::string to_string() const;


void append_to(std::string& out) const;

};


}
