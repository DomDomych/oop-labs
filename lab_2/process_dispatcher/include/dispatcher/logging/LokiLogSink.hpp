#pragma once

#include "dispatcher/logging/IHttpTransport.hpp"
#include "dispatcher/logging/ILogFormatter.hpp"
#include "dispatcher/logging/ILogSink.hpp"
#include <memory>

namespace dispatcher::logging {

class LokiLogSink final : public ILogSink {
  public:
    // URL is the full push endpoint. Fixed nonempty labels identify one stream.
    // Bounded requests in submission order. Monotonic ingestion timestamps. No implicit retries.
    LokiLogSink(std::string url, std::map<std::string, std::string> labels, std::chrono::milliseconds timeout,
                std::unique_ptr<IHttpTransport> transport, std::shared_ptr<const ILogFormatter> formatter);
    void write(std::span<const LogRecord> records) override;

  private:
    std::int64_t lastTimestampNs_ = 0;
    std::string url_;
    std::map<std::string, std::string> labels_;
    std::chrono::milliseconds timeout_;
    std::unique_ptr<IHttpTransport> transport_;
    std::shared_ptr<const ILogFormatter> formatter_;
};

} // namespace dispatcher::logging
