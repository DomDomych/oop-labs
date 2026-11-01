#pragma once

#include "logging/LogRecord.hpp"

namespace compositor::logging {

class ILogFormatter {
public:
    virtual ~ILogFormatter() = default;
    // Returns a single log line without its terminating newline.
    virtual std::string format(const LogRecord& record) const = 0;
};

} // namespace compositor::logging
