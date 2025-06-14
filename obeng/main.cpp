#include <obeng.hpp>
#include <iostream>
#include <memory>

using namespace obeng;

auto main() -> int {
    ServerOptions options;
    options.port = 9191;
    options.num_workers = 8;

    // Buat server dengan shared_ptr
    auto server = std::make_shared<HttpServer>(options);

    auto& router = server->router();  // Gunakan -> bukan .

    router.get("/", [](const HttpRequest&) -> HttpResponse {
        return HttpResponse::ok(R"({
            "message": "Hello"
        })");
    });

    std::cout << "🚀 Starting server on port " << options.port << "\n";
    std::cout << "🧵 Using " << options.num_workers << " worker threads\n";

    server->start();  // Gunakan -> bukan .

    return 0;
}
