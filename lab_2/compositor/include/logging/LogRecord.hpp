#pragma once

#include <chrono>
#include <map>
#include <string>

namespace compositor::logging {

enum class Level { Debug, Info, Warning, Error };

struct LogRecord {
    std::chrono::system_clock::time_point timestamp;
    Level level;
    std::string event;
    std::map<std::string, std::string> fields;
};

} // namespace compositor::logging
