#pragma once

#include "logging/IHttpTransport.hpp"
#include "logging/ILogFormatter.hpp"
#include "logging/ILogSink.hpp"
#include <memory>

namespace compositor::logging {

class LokiLogSink final : public ILogSink {
public:
    // URL is the full push endpoint. Fixed nonempty labels identify one stream.
    // One request per nonempty batch, sorted by timestamp. No implicit retries.
    LokiLogSink(std::string url, std::map<std::string, std::string> labels,
                std::chrono::milliseconds timeout, std::unique_ptr<IHttpTransport> transport,
                std::shared_ptr<const ILogFormatter> formatter);
    void write(std::span<const LogRecord> records) override;
private:
    std::string url_;
    std::map<std::string, std::string> labels_;
    std::chrono::milliseconds timeout_;
    std::unique_ptr<IHttpTransport> transport_;
    std::shared_ptr<const ILogFormatter> formatter_;
};

} // namespace compositor::logging
