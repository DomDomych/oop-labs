#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>

namespace compositor::logging {

struct LokiConfig {
    std::string url;
    std::map<std::string, std::string> labels{{"app", "compositor"}};
    std::chrono::milliseconds timeout{2000};
};

struct LoggingConfig {
    bool console = true;
    std::optional<std::filesystem::path> file;
    std::optional<LokiConfig> loki;
    std::size_t capacity = 1024;
};

} // namespace compositor::logging
