#pragma once

#include "logging/ILogFormatter.hpp"

namespace compositor::logging {

class JsonLogFormatter final : public ILogFormatter {
public:
    std::string format(const LogRecord& record) const override;
};

} // namespace compositor::logging
