#pragma once

#include "rendering/PixelBounds.hpp"

#include <cstddef>

namespace compositor {

// Regular geometry only: no pixel storage, locks or threads.
// IDs are row-major; edge regions are clipped to the canvas.
class RegionGrid {
public:
    // All dimensions must be positive. Region count must fit in size_t.
    RegionGrid(std::size_t width, std::size_t height,
               std::size_t regionWidth, std::size_t regionHeight);

    std::size_t columns() const noexcept;
    std::size_t rows() const noexcept;
    std::size_t count() const noexcept;
    PixelBounds bounds(std::size_t regionId) const;
    std::size_t regionAt(std::size_t x, std::size_t y) const;

private:
    std::size_t width_;
    std::size_t height_;
    std::size_t regionWidth_;
    std::size_t regionHeight_;
    std::size_t columns_;
    std::size_t rows_;
};

} // namespace compositor
