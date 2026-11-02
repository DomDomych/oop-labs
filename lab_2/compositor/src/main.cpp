#include "application/RenderApplication.hpp"
#include "application/LoggingSetup.hpp"
#include "input/JsonConfigLoader.hpp"
#include "output/PpmWriter.hpp"

#include <exception>
#include <chrono>
#include <iostream>

namespace {
constexpr int kExitSuccess = 0;
constexpr int kExitFailure = 1;
constexpr int kExitUsageError = 2;
constexpr int kExitLoggingFailure = 3;
constexpr int kExpectedArgumentCount = 2;
}

int main(int argc, char** argv) {
    const auto started = std::chrono::system_clock::now();
    const auto tick = std::chrono::steady_clock::now();
    std::unique_ptr<compositor::logging::ILogger> logger;
    int status = kExitSuccess;
    std::string failure;
    try {
        logger = compositor::createLogger({}, std::cerr);
        if (argc != kExpectedArgumentCount) {
            failure = "Usage: compositor <config.json>";
            status = kExitUsageError;
        } else {
            const auto config = compositor::JsonConfigLoader{}.load(argv[1]);
            logger = compositor::createLogger(config.logging, std::cerr);
            const compositor::PpmWriter writer;
            const compositor::RenderApplication application{writer};
            application.run(config);
        }
    } catch (const std::exception& error) {
        failure = error.what();
        status = kExitFailure;
    }
    const auto ended = std::chrono::system_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - tick);
    if (!failure.empty()) std::cerr << failure << '\n';
    try {
        if (!logger) throw std::runtime_error("Logger initialization failed");
        using namespace compositor::logging;
        logger->log({started, Level::Info, "program_started", {}});
        logger->log({ended, status == kExitSuccess ? Level::Info : Level::Error, "program_finished",
            {{"duration_ns", std::to_string(elapsed.count())}, {"exit_code", std::to_string(status)}, {"error", failure}}});
        const auto report = logger->flush();
        for (const auto& error : report.errors) std::cerr << "Logging error: " << error << '\n';
        if (report.droppedRecords != 0) std::cerr << "Dropped log records: " << report.droppedRecords << '\n';
        if (status == kExitSuccess && (!report.errors.empty() || report.droppedRecords != 0)) status = kExitLoggingFailure;
    } catch (const std::exception& error) {
        std::cerr << "Logging error: " << error.what() << '\n';
        if (status == kExitSuccess) status = kExitLoggingFailure;
    }
    return status;
}
