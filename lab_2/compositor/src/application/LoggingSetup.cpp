#include "application/LoggingSetup.hpp"
#include "logging/BufferedLogger.hpp"
#include "logging/FileLogSink.hpp"
#include "logging/JsonLogFormatter.hpp"
#include "logging/LokiLogSink.hpp"
#include "logging/StreamLogSink.hpp"
#ifdef COMPOSITOR_WITH_LOKI_HTTP
#include "logging/CurlHttpTransport.hpp"
#endif
#include <stdexcept>

namespace compositor {

std::unique_ptr<logging::ILogger> createLogger(const logging::LoggingConfig& config, std::ostream& console) {
    using namespace logging;
    auto formatter = std::make_shared<JsonLogFormatter>();
    std::vector<std::unique_ptr<ILogSink>> sinks;
    if (config.loki) {
#ifdef COMPOSITOR_WITH_LOKI_HTTP
        sinks.push_back(std::make_unique<LokiLogSink>(config.loki->url, config.loki->labels,
            config.loki->timeout, std::make_unique<CurlHttpTransport>(), formatter));
#else
        throw std::runtime_error("Loki HTTP support is disabled; build with COMPOSITOR_WITH_LOKI_HTTP=ON");
#endif
    }
    if (config.console) sinks.push_back(std::make_unique<StreamLogSink>(console, formatter));
    if (config.file) sinks.push_back(std::make_unique<FileLogSink>(*config.file, formatter));
    return std::make_unique<BufferedLogger>(std::move(sinks), config.capacity);
}

} // namespace compositor
