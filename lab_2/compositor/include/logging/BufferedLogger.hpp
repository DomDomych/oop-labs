#pragma once

#include "logging/ILogger.hpp"
#include "logging/ILogSink.hpp"
#include "logging/LoggingDefaults.hpp"
#include <atomic>
#include <memory>
#include <mutex>

namespace compositor::logging {

class BufferedLogger final : public ILogger {
public:
    // Owns sinks exclusively. Bounded by record count; overflow drops new records
    // and is reported by flush. Sink I/O happens only in flush, never in log.
    BufferedLogger(std::vector<std::unique_ptr<ILogSink>> sinks,
                   std::size_t capacity = defaults::kBufferCapacity, Level minimum = Level::Info);
    void log(LogRecord record) noexcept override;
    FlushReport flush() override;

private:
    std::vector<std::unique_ptr<ILogSink>> sinks_;
    std::size_t capacity_;
    Level minimum_;
    std::mutex bufferMutex_;
    std::mutex flushMutex_;
    std::vector<LogRecord> records_;
    std::atomic<std::size_t> dropped_{0};
};

} // namespace compositor::logging
