#pragma once

#include "logging/LogRecord.hpp"
#include <span>

namespace compositor::logging {

class ILogSink {
public:
    virtual ~ILogSink() = default;
    // Synchronous delivery attempt; does not retain the span. Throws on failure.
    // Calls must be externally serialized. Partial delivery is possible on error;
    // the caller must not assume that a failed batch can be retried without duplicates.
    virtual void write(std::span<const LogRecord> records) = 0;
};

} // namespace compositor::logging
