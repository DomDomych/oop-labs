#include "application/Fixture.hpp"
#include "dispatcher/application/Application.hpp"
#include "dispatcher/execution/Dispatchers.hpp"
#include "dispatcher/input/IConfigLoader.hpp"
#include "dispatcher/logging/BufferedLogger.hpp"
#include "dispatcher/logging/CurlHttpTransport.hpp"
#include "dispatcher/logging/FileLogSink.hpp"
#include "dispatcher/logging/JsonLogFormatter.hpp"
#include "dispatcher/logging/LokiLogSink.hpp"
#include "dispatcher/logging/StreamLogSink.hpp"
#include "platform/Platform.hpp"
#include <iostream>
#include <memory>
namespace {
class NullLogger final : public dispatcher::logging::ILogger {
  public:
    void log(dispatcher::logging::LogRecord) noexcept override {}
    dispatcher::logging::FlushReport flush() override { return {}; }
};
std::unique_ptr<dispatcher::logging::ILogger> makeLogger(const dispatcher::AppConfig &config) {
    namespace log = dispatcher::logging;
    auto formatter = std::make_shared<log::JsonLogFormatter>();
    std::vector<std::unique_ptr<log::ILogSink>> sinks;
    if (config.logging.console)
        sinks.push_back(std::make_unique<log::StreamLogSink>(std::clog, formatter));
    if (config.logging.file) {
        std::filesystem::create_directories(config.logging.file->parent_path());
        sinks.push_back(std::make_unique<log::FileLogSink>(*config.logging.file, formatter));
    }
    if (config.logging.loki) {
        const auto &loki = *config.logging.loki;
        sinks.push_back(std::make_unique<log::LokiLogSink>(
            loki.url, loki.labels, loki.timeout, std::make_unique<log::CurlHttpTransport>(), formatter));
    }
    if (sinks.empty())
        return std::make_unique<NullLogger>();
    // Two events per job + four lifecycle events. No log I/O while processes run.
    return std::make_unique<log::BufferedLogger>(std::move(sinks), config.plan.jobs().size() * 2 + 8);
}
} // namespace
int applicationMain(int argc, char **argv) {
    using namespace dispatcher;
    std::unique_ptr<logging::ILogger> logger;
    int code = 2;
    try {
        if (argc >= 2 && std::string_view(argv[1]) == "--fixture")
            return runFixture(argc, argv);
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout << "Usage: process_dispatcher config.json [--mode sync|async]\n"
                         "Fixture: --fixture [--delay-ms N] [--stdout TEXT] [--stderr TEXT] "
                         "[--bytes N] [--binary] [--exit-code N]\n";
            return 0;
        }
        if (argc != 2 && argc != 4)
            throw std::invalid_argument("Usage: process_dispatcher config.json [--mode sync|async]");
        JsonConfigLoader loader(platform::executablePath());
        auto config = loader.load(platform::fromUtf8(argv[1]));
        if (argc == 4) {
            if (std::string_view(argv[2]) != "--mode" ||
                (std::string_view(argv[3]) != "sync" && std::string_view(argv[3]) != "async"))
                throw std::invalid_argument("Expected --mode sync|async");
            config.mode = std::string_view(argv[3]) == "sync" ? Mode::Sync : Mode::Async;
        }
        logger = makeLogger(config);
        FileOutputStore outputs(config.outputDirectory);
        JsonReportWriter report(config.reportPath);
        std::unique_ptr<IDispatcher> dispatcher;
        if (config.mode == Mode::Sync)
            dispatcher = std::make_unique<SyncDispatcher>(outputs, *logger);
        else
            dispatcher = std::make_unique<AsyncDispatcher>(outputs, *logger);
        Application app(*dispatcher, report, *logger);
        code = app.run(config.plan);
        std::cout << "Report: " << platform::toUtf8(config.reportPath) << '\n';
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
    }
    if (logger) {
        try {
            auto delivered = logger->flush();
            for (const auto &error : delivered.errors)
                std::cerr << "Log delivery: " << error << '\n';
            if (delivered.droppedRecords)
                std::cerr << "Dropped log records: " << delivered.droppedRecords << '\n';
            if (!delivered.errors.empty() || delivered.droppedRecords) {
                if (code == 0)
                    code = 4;
            }
        } catch (const std::exception &error) {
            std::cerr << "Log flush failed: " << error.what() << '\n';
            if (code == 0)
                code = 4;
        }
    }
    return code;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
    try {
        auto arguments = dispatcher::platform::utf8Arguments(argc, argv);
        std::vector<char *> pointers;
        for (auto &argument : arguments)
            pointers.push_back(argument.data());
        return applicationMain(argc, pointers.data());
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
#else
int main(int argc, char **argv) {
    return applicationMain(argc, argv);
}
#endif
