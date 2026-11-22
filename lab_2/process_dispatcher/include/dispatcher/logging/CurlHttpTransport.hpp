#pragma once

#include "dispatcher/logging/IHttpTransport.hpp"

namespace dispatcher::logging {

// Optional implementation, built with COMPOSITOR_WITH_LOKI_HTTP=ON.
class CurlHttpTransport final : public IHttpTransport {
  public:
    CurlHttpTransport();
    unsigned postJson(std::string_view url, std::string_view body,
                      std::chrono::milliseconds timeout) override;
};

} // namespace dispatcher::logging
