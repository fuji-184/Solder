#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <memory>
#include <libpq-fe.h>
#include <photon/common/utility.h>
#include <photon/thread/thread11.h>
#include <photon/net/socket.h>


namespace solder {


struct Req {
    std::string method;
    std::string path;
    std::string query;
    int minor_version = 1;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::unordered_map<std::string, std::string> get_query_params() const;

    bool has_header(const std::string& name) const;
    std::string get_header(const std::string& name, const std::string& default_value = "") const;
};

struct Res {
    int status_code = 200;
    std::string status_text = "OK";
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    static Res ok(const std::string& body = "");
    static Res created(const std::string& body = "");
    static Res no_content();
    static Res bad_request(const std::string& message = "Bad Request");
    static Res not_found(const std::string& message = "Not Found");
    static Res internal_error(const std::string& message = "Internal Server Error");

    std::string to_string() const;


void append_to(std::string& out) const;

};


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
        return h1 ^ (h2 << 1);
    }
};

class HttpRouter {
public:
    using Handler = std::function<Res(const Req&)>;
    using Middleware = std::function<void(Req&, Res&, std::function<void()>)>;

    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    void put(const std::string& path, Handler handler);
    void delete_(const std::string& path, Handler handler);
    void patch(const std::string& path, Handler handler);
    void options(const std::string& path, Handler handler);

    void use(Middleware middleware);

    void group(const std::string& prefix, std::function<void(HttpRouter&)> setup);

    Res handle_request(const Req& request) const;

private:
    std::unordered_map<RouteKey, Handler, RouteKeyHash> routes_;
    std::vector<Middleware> middlewares_;

    void add_route(const std::string& method, const std::string& path, Handler handler);
    bool matches_pattern(const std::string& pattern, const std::string& actual) const;
    Res execute_with_middleware(const Req& request, const Handler& handler) const;
};



class HttpParser {
public:
    HttpParser();

    bool parse_request(const char* data, size_t len, Req& request);

    void reset();

private:
    std::vector<char> buffer_;
    size_t buffer_size_;
    size_t buffer_pos_ = 0;
    size_t last_len_ = 0;
};



struct ServerOptions {
    uint16_t port = 8080;
    size_t num_workers = 4;
    size_t buffer_size = 4096;
    bool keep_alive = true;
    std::string server_name = "SolderHTTP/1.0";
};

class HttpServer: public std::enable_shared_from_this<HttpServer> {
public:
static std::shared_ptr<HttpServer> create(const ServerOptions& options = {}) {
        return std::shared_ptr<HttpServer>(new HttpServer(options));
    }

    explicit HttpServer(const ServerOptions& options = {});

    void start();

    void set_router(std::unique_ptr<HttpRouter> router);
    HttpRouter& router();

private:
    ServerOptions options_;
    std::unique_ptr<HttpRouter> router_;


    void multiple();
    void handle_connection(photon::net::ISocketStream* stream);
};


    inline std::shared_ptr<HttpServer> make_server(const ServerOptions& options = {}) {
        return HttpServer::create(options);
    }

}
