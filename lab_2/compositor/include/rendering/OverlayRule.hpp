#pragma once

#include "model/PixelState.hpp"

namespace compositor {

// Compares candidates within one render call; the caller excludes old winners.
// Pure decision: background loses to any candidate; ties use the input index.
bool shouldReplace(const PixelState& current,
                   const RectangleMetadata& candidate) noexcept;

} // namespace compositor
