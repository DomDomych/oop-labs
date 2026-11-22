#pragma once

#include "dispatcher/logging/ILogFormatter.hpp"

namespace dispatcher::logging {

class JsonLogFormatter final : public ILogFormatter {
  public:
    std::string format(const LogRecord &record) const override;
};

} // namespace dispatcher::logging
