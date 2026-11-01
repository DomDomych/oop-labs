#pragma once

#include "logging/LoggingConfig.hpp"
#include <boost/json/value.hpp>

namespace compositor::detail {

logging::LoggingConfig parseLoggingConfig(const boost::json::value& value,
                                         const std::filesystem::path& directory);

} // namespace compositor::detail
