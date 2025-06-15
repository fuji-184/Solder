
#pragma once

#include "http_types.hpp"
#include "router.hpp"
#include "parser.hpp"
#include <photon/common/utility.h>
#include <photon/thread/thread11.h>
#include <photon/net/socket.h>
#include <memory>
#include <vector>

namespace solder {


struct ServerOptions {
    uint16_t port = 8080;
    size_t num_workers = 4;
    size_t buffer_size = 4096;
    bool keep_alive = true;
    std::string server_name = "LampuHTTP/1.0";
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
