#pragma once

#include "dispatcher/logging/ILogFormatter.hpp"
#include "dispatcher/logging/ILogSink.hpp"
#include <memory>
#include <ostream>

namespace dispatcher::logging {

class StreamLogSink final : public ILogSink {
  public:
    // Stream is borrowed and must outlive this sink; formatter is shared immutable state.
    StreamLogSink(std::ostream &stream, std::shared_ptr<const ILogFormatter> formatter);
    void write(std::span<const LogRecord> records) override;

  private:
    std::ostream &stream_;
    std::shared_ptr<const ILogFormatter> formatter_;
};

} // namespace dispatcher::logging
