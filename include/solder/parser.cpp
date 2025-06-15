#include "parser.hpp"
#include "picohttpparser.h"
#include <cstring>
#include <algorithm>

namespace solder {

HttpParser::HttpParser() : buffer_size_(8192) {
    buffer_.resize(buffer_size_);
}

bool HttpParser::parse_request(const char* data, size_t len, Req& request) {
    // Append new data to buffer
    if (buffer_pos_ + len > buffer_size_) {
        buffer_size_ = std::max(buffer_size_ * 2, buffer_pos_ + len);
        buffer_.resize(buffer_size_);
    }
    std::memcpy(buffer_.data() + buffer_pos_, data, len);
    buffer_pos_ += len;

    // Try to parse
    const char* method;
    size_t method_len;
    const char* path;
    size_t path_len;
    int minor_version;
    struct phr_header headers[64];
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    int pret = phr_parse_request(
        buffer_.data(), buffer_pos_,
        &method, &method_len,
        &path, &path_len,
        &minor_version,
        headers, &num_headers,
        last_len_
    );

    if (pret > 0) {
        // Successfully parsed
        request.method.assign(method, method_len);

        // Parse path and query
        std::string full_path(path, path_len);
        auto question_pos = full_path.find('?');
        if (question_pos != std::string::npos) {
            request.path = full_path.substr(0, question_pos);
            request.query = full_path.substr(question_pos + 1);
        } else {
            request.path = full_path;
        }

        request.minor_version = minor_version;

        // Parse headers
        for (size_t i = 0; i < num_headers; ++i) {

            request.headers.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(headers[i].name, headers[i].name_len),
    std::forward_as_tuple(headers[i].value, headers[i].value_len)
);

        }

        // Parse body if present
        size_t body_start = pret;
        if (body_start < buffer_pos_) {
            request.body.assign(buffer_.data() + body_start, buffer_pos_ - body_start);
        }

        reset();
        return true;
    } else if (pret == -1) {
        reset();
        return false;
    }

    last_len_ = buffer_pos_;
    return false;
}

void HttpParser::reset() {
    buffer_pos_ = 0;
    last_len_ = 0;
}

}
