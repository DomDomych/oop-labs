#pragma once

#include "model/PixelState.hpp"

namespace compositor {

// Pure decision: background loses to any candidate; ties use the input index.
bool shouldReplace(const PixelState& current,
                   const RectangleMetadata& candidate) noexcept;

} // namespace compositor
