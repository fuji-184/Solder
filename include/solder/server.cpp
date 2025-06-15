#include "server.hpp"
#include <photon/common/alog.h>
#include <photon/net/socket.h>
#include <photon/photon.h>
#include <photon/common/utility.h>
#include <thread>
#include <iostream>

namespace solder {

HttpServer::HttpServer(const ServerOptions& options)
    : options_(options) {}

void HttpServer::set_router(std::unique_ptr<HttpRouter> router) {
    router_ = std::move(router);
}

HttpRouter& HttpServer::router() {
    if (!router_) {
        router_ = std::make_unique<HttpRouter>();
    }
    return *router_;
}

void HttpServer::start() {
    if (!router_) {
        throw std::runtime_error("No router configured");
    }

    std::cout << "🚀 Starting server on port " << options_.port << "\n";
    std::cout << "🧵 Using " << options_.num_workers << " worker threads\n";

    std::vector<std::thread> threads;

    for (size_t i = 0; i < options_.num_workers; ++i) {
        threads.emplace_back([this] {
            // Init Photon per OS thread
            photon::init(photon::INIT_EVENT_DEFAULT, photon::INIT_IO_NONE);
            DEFER(photon::fini());

            this->multiple();
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

void HttpServer::multiple() {

    auto server = photon::net::new_tcp_socket_server();
    if (server == nullptr) {
        throw std::runtime_error("Failed to create TCP server");
    }
    DEFER(delete server);

    /*
    auto handler = [this](photon::net::ISocketStream* stream) -> int {
        DEFER(delete stream);
        stream->timeout(30UL * 1000 * 1000);
        this->handle_connection(stream);
        return 0;
    };
    */

server->setsockopt<int>(SOL_SOCKET, SO_REUSEADDR, 1);
    server->setsockopt<int>(SOL_SOCKET, SO_REUSEPORT, 1);



    auto handler = [self = shared_from_this()](photon::net::ISocketStream* stream) -> int {
        stream->timeout(30UL * 1000 * 1000);
        self->handle_connection(stream);
        return 0;
    };

    server->set_handler(handler);

    int bind_result = server->bind(options_.port, photon::net::IPAddr());
    if (bind_result != 0) {
        throw std::runtime_error("Failed to bind to localhost:" + std::to_string(options_.port));
    }

    if (server->listen() != 0) {
        throw std::runtime_error("Failed to listen on port " + std::to_string(options_.port));
    }

    LOG_INFO("Server is listening on port ", options_.port, " ...");
    LOG_INFO("Server starting main loop...");
    server->start_loop(true);



LOG_INFO("Multiple exited");


}

void HttpServer::handle_connection(photon::net::ISocketStream* stream) {
    if (!stream) {
        LOG_ERROR("Null stream in handle_connection");
        return;
    }

    try {
        HttpParser parser;
        std::string recv_buffer(options_.buffer_size, '\0');

        while (true) {
            ssize_t ret = stream->recv(recv_buffer.data(), recv_buffer.size());

            if (ret <= 0) {
                if (ret == 0) {
                    // Client closed connection gracefully
                    LOG_DEBUG("Client closed connection gracefully");
                } else {
                    // Check errno for specific error
                    int error = errno;
                    if (error == ECONNRESET) {
                        LOG_DEBUG("Connection reset by peer");
                    } else if (error == ETIMEDOUT) {
                        LOG_DEBUG("Connection timed out");
                    } else if (error == EAGAIN || error == EWOULDBLOCK) {
                        LOG_DEBUG("Would block - no data available");
                    } else {
                        LOG_DEBUG("Recv error: ", ret, " errno: ", error);
                    }
                }
                break;
            }

            Req request;
            bool parsed = parser.parse_request(recv_buffer.data(), ret, request);

            if (parsed) {
                // Check if router is still valid
                if (!router_) {
                    LOG_ERROR("Router is null in handle_connection");
                    break;
                }

                Res response;
                try {
                    response = router_->handle_request(request);
                } catch (const std::exception& e) {
                    LOG_ERROR("Error handling request: ", e.what());
                    response = Res::internal_error("Internal Server Error");
                }

                std::string response_str;
                response_str.reserve(512 + response.body.size());
                response.append_to(response_str);

                ssize_t sent = stream->send(response_str.data(), response_str.size());
                if (sent != static_cast<ssize_t>(response_str.size())) {
                    LOG_DEBUG("Failed to send complete response, sent: ", sent, "/", response_str.size());
                    break;
                }

                // Check for connection close
                auto it = request.headers.find("Connection");
                if (request.minor_version == 0 ||
                    (it != request.headers.end() && it->second == "close")) {
                    LOG_DEBUG("Connection close requested");
                    break;
                }
                parser.reset();
            } else {
                LOG_DEBUG("Failed to parse request, closing connection");
                break;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in handle_connection: ", e.what());
    } catch (...) {
        LOG_ERROR("Unknown exception in handle_connection");
    }
}


}
