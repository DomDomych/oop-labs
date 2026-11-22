#pragma once

#include <chrono>
#include <cstddef>

namespace dispatcher::logging::defaults {

inline constexpr std::size_t kBufferCapacity = 1024;
inline constexpr std::chrono::milliseconds kLokiTimeout{2000};

} // namespace dispatcher::logging::defaults
