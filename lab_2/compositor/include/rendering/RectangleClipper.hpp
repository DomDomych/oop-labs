#pragma once

#include "model/Rectangle.hpp"

#include <cstddef>
#include <optional>

namespace compositor {

// Half-open bounds: [left, right) x [top, bottom).
struct PixelBounds {
    std::size_t left;
    std::size_t top;
    std::size_t right;
    std::size_t bottom;

    bool operator==(const PixelBounds&) const = default;
};

class RectangleClipper {
public:
    // Returns no bounds for an empty intersection, including an empty canvas.
    // Does not access pixels or modify the rectangle.
    std::optional<PixelBounds> clip(const Rectangle& rectangle,
                                    std::size_t width,
                                    std::size_t height) const noexcept;
};

} // namespace compositor
