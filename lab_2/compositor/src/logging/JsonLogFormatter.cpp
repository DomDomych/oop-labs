#include "logging/JsonLogFormatter.hpp"

#include <boost/json.hpp>
#include <stdexcept>

namespace compositor::logging {

std::string JsonLogFormatter::format(const LogRecord& record) const {
    const char* level;
    switch (record.level) {
    case Level::Debug: level = "debug"; break;
    case Level::Info: level = "info"; break;
    case Level::Warning: level = "warning"; break;
    case Level::Error: level = "error"; break;
    default: throw std::invalid_argument("Unknown log level");
    }
    boost::json::object fields;
    for (const auto& [key, value] : record.fields) fields[key] = value;
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(record.timestamp.time_since_epoch()).count();
    return boost::json::serialize(boost::json::object{
        {"timestamp_ns", std::to_string(ns)}, {"level", level},
        {"event", record.event}, {"fields", std::move(fields)}});
}

} // namespace compositor::logging
