#include "logging/FileLogSink.hpp"
#include <stdexcept>

namespace compositor::logging {

FileLogSink::FileLogSink(const std::filesystem::path& path, std::shared_ptr<const ILogFormatter> formatter)
    : file_(path, std::ios::binary | std::ios::app), streamSink_(file_, std::move(formatter)) {
    if (!file_) throw std::runtime_error("Cannot open log file: " + path.string());
}
void FileLogSink::write(std::span<const LogRecord> records) { streamSink_.write(records); }

} // namespace compositor::logging
