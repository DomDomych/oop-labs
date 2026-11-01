#pragma once

#include <chrono>
#include <string_view>

namespace compositor::logging {

class IHttpTransport {
public:
    virtual ~IHttpTransport() = default;
    // Synchronous JSON POST. Throws on transport failure; returns HTTP status.
    // Must bound network waiting by timeout and must not retain input views.
    virtual unsigned postJson(std::string_view url, std::string_view body,
                              std::chrono::milliseconds timeout) = 0;
};

} // namespace compositor::logging
