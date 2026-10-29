#include "rendering/Compositor.hpp"

#include "rendering/OverlayRule.hpp"
#include "rendering/RectangleClipper.hpp"
#include "rendering/RegionGrid.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace compositor {

Compositor::Compositor(std::size_t regionWidth, std::size_t regionHeight)
    : regionWidth_(regionWidth), regionHeight_(regionHeight) {
    if (regionWidth == 0 || regionHeight == 0) {
        throw std::invalid_argument("Region dimensions must be positive");
    }
}

void Compositor::render(const Scene& scene, Image& canvas) const {
    const RectangleClipper clipper;
    const auto& rectangles = scene.rectangles();
    if (rectangles.empty()) {
        return;
    }
    Image working = canvas;
    const RegionGrid grid{canvas.width(), canvas.height(), regionWidth_, regionHeight_};
    // Call-local bookkeeping: old winner metadata remains intact until overwritten.
    // Image construction has already validated the pixel count.
    std::vector<std::uint8_t> touched(canvas.width() * canvas.height(), 0);
    for (std::size_t index = 0; index < rectangles.size(); ++index) {
        const auto& rectangle = rectangles[index];
        const auto bounds = clipper.clip(rectangle, canvas.width(), canvas.height());
        if (!bounds) {
            continue;
        }
        const RectangleMetadata candidate{rectangle.priority(), index};
        const PixelState replacement{rectangle.color(), candidate};
        // Only enumerate regions intersecting this rectangle, not the whole grid.
        const auto first = grid.regionAt(bounds->left, bounds->top);
        const auto last = grid.regionAt(bounds->right - 1, bounds->bottom - 1);
        const auto firstColumn = first % grid.columns();
        const auto lastColumn = last % grid.columns();
        const auto lastRow = last / grid.columns();
        for (auto row = first / grid.columns(); row <= lastRow; ++row) {
            for (auto column = firstColumn; column <= lastColumn; ++column) {
                const auto region = grid.bounds(row * grid.columns() + column);
                const PixelBounds intersection{
                    std::max(bounds->left, region.left),
                    std::max(bounds->top, region.top),
                    std::min(bounds->right, region.right),
                    std::min(bounds->bottom, region.bottom)};
                // This complete intersection will be protected by one region lock.
                for (auto y = intersection.top; y < intersection.bottom; ++y) {
                    for (auto x = intersection.left; x < intersection.right; ++x) {
                        auto& visited = touched[y * canvas.width() + x];
                        if (!visited || shouldReplace(working.at(x, y), candidate)) {
                            working.setPixel(x, y, replacement);
                        }
                        visited = 1;
                    }
                }
            }
        }
    }
    canvas.swap(working);
}

} // namespace compositor
