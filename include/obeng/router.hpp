#pragma once
#include "http_types.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

namespace obeng {

struct RouteKey {
    std::string method;
    std::string path;

    bool operator==(const RouteKey& other) const {
        return method == other.method && path == other.path;
    }
};

struct RouteKeyHash {
    std::size_t operator()(const RouteKey& key) const {
        std::size_t h1 = std::hash<std::string>{}(key.method);
        std::size_t h2 = std::hash<std::string>{}(key.path);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }
};

class HttpRouter {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;
    using Middleware = std::function<void(HttpRequest&, HttpResponse&, std::function<void()>)>;

    // Route registration methods
    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    void put(const std::string& path, Handler handler);
    void delete_(const std::string& path, Handler handler);
    void patch(const std::string& path, Handler handler);
    void options(const std::string& path, Handler handler);

    // Middleware support
    void use(Middleware middleware);

    // Route groups
    void group(const std::string& prefix, std::function<void(HttpRouter&)> setup);

    HttpResponse handle_request(const HttpRequest& request) const;

private:
    std::unordered_map<RouteKey, Handler, RouteKeyHash> routes_;
    std::vector<Middleware> middlewares_;

    void add_route(const std::string& method, const std::string& path, Handler handler);
    bool matches_pattern(const std::string& pattern, const std::string& actual) const;
    HttpResponse execute_with_middleware(const HttpRequest& request, const Handler& handler) const;
};

}
