
#pragma once

#include "http_types.hpp"
#include "router.hpp"
#include "parser.hpp"
#include <photon/common/utility.h>
#include <photon/thread/thread11.h>
#include <photon/net/socket.h>
#include <memory>
#include <vector>

namespace obeng {

struct ServerOptions {
    uint16_t port = 8080;
    size_t num_workers = 4;
    size_t buffer_size = 4096;
    bool keep_alive = true;
    std::string server_name = "PhotonHTTP/1.0";
};

class HttpServer: public std::enable_shared_from_this<HttpServer> {
public:
    explicit HttpServer(const ServerOptions& options = {});

    void start();
 void start_single_acceptor();  // Single thread accepts, creates workers
    void start_multiple_acceptor(); // Each worker has own server socket


    void set_router(std::unique_ptr<HttpRouter> router);
    HttpRouter& router();

private:
    ServerOptions options_;
    std::unique_ptr<HttpRouter> router_;


void multiple();

   void worker_with_own_server(size_t worker_id);
    void handle_connection(photon::net::ISocketStream* stream);



};

}
