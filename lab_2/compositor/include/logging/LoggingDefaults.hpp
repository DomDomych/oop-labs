#pragma once

#include <chrono>
#include <cstddef>

namespace compositor::logging::defaults {

inline constexpr std::size_t kBufferCapacity = 1024;
inline constexpr std::chrono::milliseconds kLokiTimeout{2000};

} // namespace compositor::logging::defaults
