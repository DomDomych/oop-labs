#pragma once

#include "logging/LogRecord.hpp"
#include <cstddef>
#include <vector>

namespace compositor::logging {

struct FlushReport {
    std::size_t droppedRecords = 0;
    std::vector<std::string> errors;
};

class ILogger {
public:
    virtual ~ILogger() = default;
    // Records are owned by the logger after submission. Implementations must
    // support concurrent calls and never throw from log(). Argument construction
    // happens in the caller and is not covered by this guarantee.
    virtual void log(LogRecord record) noexcept = 0;
    // Completes delivery attempts for the current batch. Reports sink failures.
    // May allocate/throw; call explicitly before destruction. No implicit I/O
    // from destructors. Stop producers first to flush every submitted record.
    virtual FlushReport flush() = 0;
};

} // namespace compositor::logging
