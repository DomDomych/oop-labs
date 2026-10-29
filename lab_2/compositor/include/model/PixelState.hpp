#pragma once

#include "model/Color.hpp"

#include <cstddef>
#include <optional>

namespace compositor {

struct RectangleMetadata {
    int priority;
    // Position in the scene of the render call that last changed this pixel.
    std::size_t index;

    bool operator==(const RectangleMetadata&) const = default;
};

struct PixelState {
    Color color;
    // No winner means background, even when rectangle priorities are negative.
    std::optional<RectangleMetadata> winner = std::nullopt;

    bool operator==(const PixelState&) const = default;
};

} // namespace compositor
