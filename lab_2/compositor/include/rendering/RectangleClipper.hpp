#pragma once

#include "model/Rectangle.hpp"
#include "rendering/PixelBounds.hpp"

#include <cstddef>
#include <optional>

namespace compositor {

class RectangleClipper {
public:
    // Returns no bounds for an empty intersection, including an empty canvas.
    // Does not access pixels or modify the rectangle.
    std::optional<PixelBounds> clip(const Rectangle& rectangle,
                                    std::size_t width,
                                    std::size_t height) const noexcept;
};

} // namespace compositor
