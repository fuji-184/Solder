#pragma once
#include "http_types.hpp"
#include <vector>

namespace obeng {

class HttpParser {
public:
    HttpParser();

    bool parse_request(const char* data, size_t len, HttpRequest& request);

    void reset();

private:
    std::vector<char> buffer_;
    size_t buffer_size_;
    size_t buffer_pos_ = 0;
    size_t last_len_ = 0;
};

}
