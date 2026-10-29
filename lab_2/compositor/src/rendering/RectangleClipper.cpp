#include "rendering/RectangleClipper.hpp"

#include <cstdint>
#include <utility>

namespace compositor {
namespace {

std::size_t clampBoundary(std::int64_t value, std::size_t limit) noexcept {
    if (value <= 0) {
        return 0;
    }
    if (std::cmp_greater_equal(value, limit)) {
        return limit;
    }
    return static_cast<std::size_t>(value);
}

} // namespace

std::optional<PixelBounds> RectangleClipper::clip(
    const Rectangle& rectangle, std::size_t width, std::size_t height) const noexcept {
    const PixelBounds bounds{
        clampBoundary(rectangle.x(), width),
        clampBoundary(rectangle.y(), height),
        clampBoundary(rectangle.right(), width),
        clampBoundary(rectangle.bottom(), height)};
    if (bounds.left >= bounds.right || bounds.top >= bounds.bottom) {
        return std::nullopt;
    }
    return bounds;
}

} // namespace compositor
