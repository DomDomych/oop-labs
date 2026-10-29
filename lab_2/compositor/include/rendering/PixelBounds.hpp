#pragma once

#include <cstddef>

namespace compositor {

// Half-open bounds: [left, right) x [top, bottom).
struct PixelBounds {
    std::size_t left;
    std::size_t top;
    std::size_t right;
    std::size_t bottom;

    bool operator==(const PixelBounds&) const = default;
};

} // namespace compositor
