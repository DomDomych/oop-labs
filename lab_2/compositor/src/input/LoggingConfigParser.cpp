#include "LoggingConfigParser.hpp"

#include <boost/json.hpp>
#include <algorithm>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace compositor::detail {
namespace {
constexpr std::int64_t kMinimumConfiguredCapacity = 2;
constexpr std::int64_t kMaximumConfiguredCapacity = 1'000'000;
constexpr std::int64_t kMinimumLokiTimeoutMilliseconds = 1;
constexpr std::int64_t kMaximumLokiTimeoutMilliseconds = 60'000;

[[noreturn]] void fail(const std::string& field) { throw std::runtime_error("logging." + field + ": invalid value"); }
void keys(const boost::json::value& value, std::initializer_list<std::string_view> allowed, const std::string& field) {
    if (!value.is_object()) fail(field);
    for (const auto& pair : value.as_object()) {
        const std::string_view key{pair.key().data(), pair.key().size()};
        if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) fail(field + "." + std::string(key));
    }
}
std::string text(const boost::json::value& value, const std::string& field) {
    if (!value.is_string() || value.as_string().empty()) fail(field);
    std::string result(value.as_string());
    if (result.find('\0') != std::string::npos) fail(field);
    return result;
}
std::int64_t number(const boost::json::value& value, const std::string& field, std::int64_t min, std::int64_t max) {
    if (!value.is_int64() || value.as_int64() < min || value.as_int64() > max) fail(field);
    return value.as_int64();
}
}

logging::LoggingConfig parseLoggingConfig(const boost::json::value& value,
                                         const std::filesystem::path& directory) {
    keys(value, {"console", "file", "loki", "capacity"}, "config");
    const auto& object = value.as_object();
    logging::LoggingConfig config;
    if (const auto* console = object.if_contains("console")) {
        if (!console->is_bool()) fail("console");
        config.console = console->as_bool();
    }
    if (const auto* file = object.if_contains("file"))
        config.file = (directory / text(*file, "file")).lexically_normal();
    if (const auto* capacity = object.if_contains("capacity"))
        config.capacity = static_cast<std::size_t>(number(*capacity, "capacity", kMinimumConfiguredCapacity,
                                                          kMaximumConfiguredCapacity));
    if (const auto* loki = object.if_contains("loki")) {
        keys(*loki, {"url", "labels", "timeout_ms"}, "loki");
        const auto& settings = loki->as_object();
        const auto* url = settings.if_contains("url");
        if (!url) fail("loki.url");
        logging::LokiConfig options;
        options.url = text(*url, "loki.url");
        if (!options.url.starts_with("http://") && !options.url.starts_with("https://")) fail("loki.url");
        if (const auto* timeout = settings.if_contains("timeout_ms"))
            options.timeout = std::chrono::milliseconds{number(*timeout, "loki.timeout_ms",
                kMinimumLokiTimeoutMilliseconds, kMaximumLokiTimeoutMilliseconds)};
        if (const auto* labels = settings.if_contains("labels")) {
            if (!labels->is_object() || labels->as_object().empty()) fail("loki.labels");
            options.labels.clear();
            for (const auto& pair : labels->as_object()) {
                const std::string name(pair.key());
                const auto letter = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; };
                if (name.empty() || !letter(name.front()) || !std::all_of(name.begin(), name.end(), [&](char c) {
                    return letter(c) || (c >= '0' && c <= '9');
                })) fail("loki.labels");
                options.labels[name] = text(pair.value(), "loki.labels." + name);
            }
        }
        config.loki = std::move(options);
    }
    if (!config.console && !config.file && !config.loki) fail("config (no outputs)");
    return config;
}

} // namespace compositor::detail
