#include "logging/BufferedLogger.hpp"

#include <stdexcept>

namespace compositor::logging {

BufferedLogger::BufferedLogger(std::vector<std::unique_ptr<ILogSink>> sinks,
                               std::size_t capacity, Level minimum)
    : sinks_(std::move(sinks)), capacity_(capacity), minimum_(minimum) {
    if (capacity == 0 || sinks_.empty()) throw std::invalid_argument("Logger needs positive capacity and at least one sink");
    for (const auto& sink : sinks_) if (!sink) throw std::invalid_argument("Null log sink");
    if (minimum < Level::Debug || minimum > Level::Error) throw std::invalid_argument("Invalid minimum log level");
}

void BufferedLogger::log(LogRecord record) noexcept {
    if (record.level < minimum_) return;
    try {
        const std::lock_guard lock(bufferMutex_);
        if (records_.size() == capacity_) { ++dropped_; return; }
        records_.push_back(std::move(record));
    } catch (...) { ++dropped_; }
}

FlushReport BufferedLogger::flush() {
    const std::lock_guard flushLock(flushMutex_);
    FlushReport report;
    report.errors.reserve(sinks_.size());
    std::vector<LogRecord> batch;
    {
        const std::lock_guard lock(bufferMutex_);
        batch.swap(records_);
        report.droppedRecords = dropped_.exchange(0);
    }
    if (batch.empty()) return report;
    for (std::size_t i = 0; i < sinks_.size(); ++i) {
        try { sinks_[i]->write(batch); }
        catch (const std::exception& error) {
            report.errors.push_back("sink[" + std::to_string(i) + "]: " + error.what());
        } catch (...) { report.errors.push_back("sink[" + std::to_string(i) + "]: unknown failure"); }
    }
    return report;
}

} // namespace compositor::logging
