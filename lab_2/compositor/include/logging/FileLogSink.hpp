#pragma once

#include "logging/StreamLogSink.hpp"
#include <filesystem>
#include <fstream>

namespace compositor::logging {

class FileLogSink final : public ILogSink {
public:
    // Appends JSONL (or another injected line format); does not create directories.
    FileLogSink(const std::filesystem::path& path, std::shared_ptr<const ILogFormatter> formatter);
    FileLogSink(const FileLogSink&) = delete;
    FileLogSink& operator=(const FileLogSink&) = delete;
    FileLogSink(FileLogSink&&) = delete;
    FileLogSink& operator=(FileLogSink&&) = delete;
    void write(std::span<const LogRecord> records) override;
private:
    std::ofstream file_;
    StreamLogSink streamSink_;
};

} // namespace compositor::logging
