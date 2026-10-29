#include "rendering/RegionGrid.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace compositor {

RegionGrid::RegionGrid(std::size_t width, std::size_t height,
                       std::size_t regionWidth, std::size_t regionHeight)
    : width_(width), height_(height), regionWidth_(regionWidth),
      regionHeight_(regionHeight), columns_(0), rows_(0) {
    if (width == 0 || height == 0 || regionWidth == 0 || regionHeight == 0) {
        throw std::invalid_argument("Canvas and region dimensions must be positive");
    }
    // Ceiling division without adding potentially overflowing dimensions.
    columns_ = 1 + (width - 1) / regionWidth;
    rows_ = 1 + (height - 1) / regionHeight;
    if (columns_ > std::numeric_limits<std::size_t>::max() / rows_) {
        throw std::length_error("Region count exceeds size_t capacity");
    }
}

std::size_t RegionGrid::columns() const noexcept { return columns_; }

std::size_t RegionGrid::rows() const noexcept { return rows_; }

std::size_t RegionGrid::count() const noexcept { return columns_ * rows_; }

PixelBounds RegionGrid::bounds(std::size_t regionId) const {
    if (regionId >= count()) {
        throw std::out_of_range("Region index is outside the grid");
    }
    const auto left = (regionId % columns_) * regionWidth_;
    const auto top = (regionId / columns_) * regionHeight_;
    return {left, top,
            left + std::min(regionWidth_, width_ - left),
            top + std::min(regionHeight_, height_ - top)};
}

std::size_t RegionGrid::regionAt(std::size_t x, std::size_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("Pixel coordinates are outside the grid");
    }
    return (y / regionHeight_) * columns_ + x / regionWidth_;
}

} // namespace compositor
