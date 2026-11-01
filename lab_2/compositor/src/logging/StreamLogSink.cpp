#include "logging/StreamLogSink.hpp"
#include <stdexcept>

namespace compositor::logging {

StreamLogSink::StreamLogSink(std::ostream& stream, std::shared_ptr<const ILogFormatter> formatter)
    : stream_(stream), formatter_(std::move(formatter)) {
    if (!formatter_) throw std::invalid_argument("Null log formatter");
}

void StreamLogSink::write(std::span<const LogRecord> records) {
    for (const auto& record : records) stream_ << formatter_->format(record) << '\n';
    stream_.flush();
    if (!stream_) throw std::runtime_error("Log stream write failed");
}

} // namespace compositor::logging
