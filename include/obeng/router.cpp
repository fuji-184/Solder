#include "router.hpp"

namespace obeng {

void HttpRouter::get(const std::string& path, Handler handler) {
    add_route("GET", path, std::move(handler));
}

void HttpRouter::post(const std::string& path, Handler handler) {
    add_route("POST", path, std::move(handler));
}

void HttpRouter::put(const std::string& path, Handler handler) {
    add_route("PUT", path, std::move(handler));
}

void HttpRouter::delete_(const std::string& path, Handler handler) {
    add_route("DELETE", path, std::move(handler));
}

void HttpRouter::patch(const std::string& path, Handler handler) {
    add_route("PATCH", path, std::move(handler));
}

void HttpRouter::options(const std::string& path, Handler handler) {
    add_route("OPTIONS", path, std::move(handler));
}

void HttpRouter::use(Middleware middleware) {
    middlewares_.push_back(std::move(middleware));
}

void HttpRouter::group(const std::string& prefix, std::function<void(HttpRouter&)> setup) {
    HttpRouter sub_router;
    setup(sub_router);

    for (const auto& [route_key, handler] : sub_router.routes_) {
    const std::string& method = route_key.method;
    const std::string& path = route_key.path;
    add_route(method, prefix + path, handler);
}


}

HttpResponse HttpRouter::handle_request(const HttpRequest& request) const {
    RouteKey key{request.method, request.path};

    auto it = routes_.find(key);
    if (it != routes_.end()) {
        return execute_with_middleware(request, it->second);
    }

    for (const auto& [pattern_key, handler] : routes_) {
        std::string pattern = pattern_key.method + " " + pattern_key.path;
        std::string actual = request.method + " " + request.path;

        if (matches_pattern(pattern, actual)) {
            return execute_with_middleware(request, handler);
        }
    }

    return HttpResponse::not_found("The requested resource was not found");
}

void HttpRouter::add_route(const std::string& method, const std::string& path, Handler handler) {
    routes_[RouteKey{method, path}] = std::move(handler);
}

bool HttpRouter::matches_pattern(const std::string& pattern, const std::string& actual) const {
    if (pattern.find('*') == std::string::npos) {
        return false;
    }

    std::string prefix = pattern.substr(0, pattern.find('*'));
    return actual.length() >= prefix.length() &&
           actual.substr(0, prefix.length()) == prefix;
}

HttpResponse HttpRouter::execute_with_middleware(const HttpRequest& request, const Handler& handler) const {
    // For now, just execute handler directly
    // In a full implementation, you'd chain middleware here
    return handler(request);
}

}
