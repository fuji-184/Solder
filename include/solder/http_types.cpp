#include "http_types.hpp"
#include <sstream>

namespace solder {

// HttpRequest implementations
std::unordered_map<std::string, std::string> Req::get_query_params() const {
    std::unordered_map<std::string, std::string> params;
    if (query.empty()) return params;

    std::istringstream iss(query);
    std::string pair;
    while (std::getline(iss, pair, '&')) {
        auto eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            params[key] = value;
        }
    }
    return params;
}

bool Req::has_header(const std::string& name) const {
    return headers.find(name) != headers.end();
}

std::string Req::get_header(const std::string& name, const std::string& default_value) const {
    auto it = headers.find(name);
    return it != headers.end() ? it->second : default_value;
}

// HttpResponse implementations
Res Res::ok(const std::string& body) {
    return {200, "OK", {}, std::move(body)};
}

Res Res::created(const std::string& body) {
    return {201, "Created", {}, std::move(body)};
}

Res Res::no_content() {
    return {204, "No Content", {}, ""};
}

Res Res::bad_request(const std::string& message) {
    return {400, "Bad Request", {{"Content-Type", "application/json"}},
            R"({"error": "Bad Request", "message": ")" + std::move(message) + R"("})"};
}

Res Res::not_found(const std::string& message) {
    return {404, "Not Found", {{"Content-Type", "application/json"}},
            R"({"error": "Not Found", "message": ")" + std::move(message) + R"("})"};
}

Res Res::internal_error(const std::string& message) {
    return {500, "Internal Server Error", {{"Content-Type", "application/json"}},
            R"({"error": "Internal Server Error", "message": ")" + std::move(message) + R"("})"};
}

std::string Res::to_string() const {
    std::string response = "HTTP/1.1 " + std::to_string(status_code) + " " + status_text + "\r\n";

    // Tambah headers yang sudah ada
    bool has_content_type = false;
    bool has_connection = false;
    bool has_server = false;

    for (const auto& [key, value] : headers) {
        response += key + ": " + value + "\r\n";

        // Tracking keys
        if (key == "Content-Type") has_content_type = true;
        else if (key == "Connection") has_connection = true;
        else if (key == "Server") has_server = true;
    }

    // Tambah default headers jika belum ada
    if (!has_content_type) response += "Content-Type: application/json\r\n";
    if (!has_connection)    response += "Connection: keep-alive\r\n";
    if (!has_server)        response += "Server: PicoHTTP/2.0\r\n";

    // Content-Length
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";

    response += "\r\n" + body;
    return response;
}

void Res::append_to(std::string& response) const {

    response = "HTTP/1.1 " + std::to_string(status_code) + " " + status_text + "\r\n";

    // Tambah headers yang sudah ada
    bool has_content_type = false;
    bool has_connection = false;
    bool has_server = false;

    for (const auto& [key, value] : headers) {
        response += key + ": " + value + "\r\n";

        // Tracking keys
        if (key == "Content-Type") has_content_type = true;
        else if (key == "Connection") has_connection = true;
        else if (key == "Server") has_server = true;
    }

    // Tambah default headers jika belum ada
    if (!has_content_type) response += "Content-Type: application/json\r\n";
    if (!has_connection)    response += "Connection: keep-alive\r\n";
    if (!has_server)        response += "Server: PicoHTTP/2.0\r\n";

    // Content-Length
    response += "Content-Length: " + std::to_string(body.length()) + "\r\n";

    response += "\r\n" + body;


}


}
