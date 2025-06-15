#include <solder.hpp>

auto main() -> int {
    using namespace solder;

    auto server = make_server({
        .port = 9191,
        .num_workers = 8
    });

    auto& router = server->router();

    router.get("/", [](const Req&) -> Res {
        return Res::ok(R"({
            "message": "Hello"
        })");
    });

    server->start();

    return 0;
}
