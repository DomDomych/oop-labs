#pragma once

#include "logging/ILogger.hpp"
#include "logging/LoggingConfig.hpp"
#include <memory>
#include <ostream>

namespace compositor {

// Composition root for logging. The console stream must outlive the logger.
std::unique_ptr<logging::ILogger> createLogger(const logging::LoggingConfig& config,
                                             std::ostream& console);

} // namespace compositor
