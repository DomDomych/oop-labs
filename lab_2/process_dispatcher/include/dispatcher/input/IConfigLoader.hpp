#pragma once
#include "dispatcher/model/Model.hpp"
#include <map>
namespace dispatcher {
enum class Mode { Sync, Async };
struct LokiOptions {
    std::string url;
    std::chrono::milliseconds timeout{2000};
    std::map<std::string, std::string> labels{{"app", "process_dispatcher"}};
};
struct LoggingOptions {
    bool console = true;
    std::optional<std::filesystem::path> file;
    std::optional<LokiOptions> loki;
};
struct AppConfig {
    ExecutionPlan plan;
    Mode mode;
    std::filesystem::path reportPath, outputDirectory;
    LoggingOptions logging;
};
class IConfigLoader {
  public:
    virtual ~IConfigLoader() = default;
    virtual AppConfig load(const std::filesystem::path &path) const = 0;
};
class JsonConfigLoader final : public IConfigLoader {
  public:
    explicit JsonConfigLoader(std::filesystem::path self) : self_(std::move(self)) {}
    AppConfig load(const std::filesystem::path &path) const override;

  private:
    std::filesystem::path self_;
};
} // namespace dispatcher
