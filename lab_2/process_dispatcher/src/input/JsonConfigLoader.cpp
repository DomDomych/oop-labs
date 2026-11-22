#include "dispatcher/input/IConfigLoader.hpp"
#include "platform/Platform.hpp"
#include <boost/json.hpp>
#include <fstream>
#include <set>
#include <stdexcept>
namespace dispatcher {
namespace {
namespace json = boost::json;
void keys(const json::object &object, std::initializer_list<std::string_view> allowed) {
    for (const auto &item : object) {
        bool found = false;
        for (auto key : allowed)
            if (item.key() == key)
                found = true;
        if (!found)
            throw std::invalid_argument("Unknown configuration field: " + std::string(item.key()));
    }
}
std::string string(const json::value &value) {
    const auto &s = value.as_string();
    return {s.data(), s.size()};
}
std::size_t number(const json::value &value, std::size_t max) {
    std::uint64_t n;
    if (value.is_uint64())
        n = value.as_uint64();
    else if (value.is_int64() && value.as_int64() > 0)
        n = static_cast<std::uint64_t>(value.as_int64());
    else
        throw std::invalid_argument("Expected positive integer");
    if (n == 0 || n > max)
        throw std::invalid_argument("Integer outside supported range");
    return static_cast<std::size_t>(n);
}
std::filesystem::path path(const std::string &value, const std::filesystem::path &base) {
    if (value.empty() || value.find('\0') != std::string::npos)
        throw std::invalid_argument("Invalid path");
    return std::filesystem::absolute(base / platform::fromUtf8(value)).lexically_normal();
}
} // namespace
AppConfig JsonConfigLoader::load(const std::filesystem::path &configPath) const {
    std::ifstream file(configPath, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open config: " + platform::toUtf8(configPath));
    std::string data;
    char block[8192];
    while (file.read(block, sizeof(block)) || file.gcount()) {
        data.append(block, static_cast<std::size_t>(file.gcount()));
        if (data.size() > 8 * 1024 * 1024)
            throw std::invalid_argument("Config exceeds 8 MiB");
    }
    if (file.bad())
        throw std::runtime_error("Cannot read configuration");
    const auto document = json::parse(data);
    const auto &root = document.as_object();
    keys(root, {"mode", "max_processes", "block_size", "jobs", "report", "output_directory", "logging"});
    const auto modeName = string(root.at("mode"));
    if (modeName != "sync" && modeName != "async")
        throw std::invalid_argument("Mode must be sync or async");
    const auto base = std::filesystem::absolute(configPath).parent_path();
    auto report = path(string(root.at("report")), base);
    auto output = root.if_contains("output_directory") ? path(string(root.at("output_directory")), base)
                                                       : std::filesystem::path(report).concat(".outputs");
    std::vector<Job> jobs;
    for (const auto &entry : root.at("jobs").as_array()) {
        const auto &object = entry.as_object();
        keys(object, {"id", "executable", "arguments"});
        const auto exe = string(object.at("executable"));
        std::vector<std::string> arguments;
        for (const auto &arg : object.at("arguments").as_array())
            arguments.push_back(string(arg));
        jobs.emplace_back(string(object.at("id")), exe == "@self" ? self_ : path(exe, base),
                          std::move(arguments));
    }
    ExecutionPlan plan(std::move(jobs), number(root.at("max_processes"), 256),
                       root.if_contains("block_size") ? number(root.at("block_size"), 1048576) : 65536);
    LoggingOptions logging;
    if (const auto *value = root.if_contains("logging")) {
        const auto &object = value->as_object();
        keys(object, {"console", "file", "loki"});
        if (const auto *item = object.if_contains("console"))
            logging.console = item->as_bool();
        if (const auto *item = object.if_contains("file"))
            logging.file = path(string(*item), base);
        if (const auto *item = object.if_contains("loki")) {
            const auto &options = item->as_object();
            keys(options, {"url", "labels", "timeout_ms"});
            LokiOptions loki;
            loki.url = string(options.at("url"));
            if (const auto *timeout = options.if_contains("timeout_ms"))
                loki.timeout = std::chrono::milliseconds(number(*timeout, 60000));
            if (const auto *labels = options.if_contains("labels")) {
                loki.labels.clear();
                for (const auto &label : labels->as_object())
                    loki.labels[std::string(label.key())] = string(label.value());
            }
            logging.loki = std::move(loki);
        }
    }
    if (logging.file && *logging.file == report)
        throw std::invalid_argument("Report and log file must differ");
    for (const auto &job : plan.jobs()) {
        if (job.executable() == report || (logging.file && job.executable() == *logging.file))
            throw std::invalid_argument("Output path overlaps an executable");
    }
    return {std::move(plan), modeName == "sync" ? Mode::Sync : Mode::Async, std::move(report),
            std::move(output), std::move(logging)};
}
} // namespace dispatcher
