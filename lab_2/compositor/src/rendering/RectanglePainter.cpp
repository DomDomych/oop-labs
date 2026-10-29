#include "RectanglePainter.hpp"

#include "rendering/OverlayRule.hpp"
#include "rendering/RectangleClipper.hpp"

#include <algorithm>

namespace compositor::detail {

RectanglePainter::RectanglePainter(Image& working, std::size_t regionWidth,
                                   std::size_t regionHeight)
    : working_(working), grid_(working.width(), working.height(), regionWidth, regionHeight),
      touched_(working.width() * working.height(), 0) {}

void RectanglePainter::paint(const Rectangle& rectangle, std::size_t index) {
    const auto bounds = RectangleClipper{}.clip(rectangle, working_.width(), working_.height());
    if (!bounds) {
        return;
    }
    const PixelState replacement{rectangle.color(), RectangleMetadata{rectangle.priority(), index}};
    const auto first = grid_.regionAt(bounds->left, bounds->top);
    const auto last = grid_.regionAt(bounds->right - 1, bounds->bottom - 1);
    const auto firstColumn = first % grid_.columns();
    const auto lastColumn = last % grid_.columns();
    const auto lastRow = last / grid_.columns();
    for (auto row = first / grid_.columns(); row <= lastRow; ++row) {
        for (auto column = firstColumn; column <= lastColumn; ++column) {
            const auto region = grid_.bounds(row * grid_.columns() + column);
            paintRegion({std::max(bounds->left, region.left),
                         std::max(bounds->top, region.top),
                         std::min(bounds->right, region.right),
                         std::min(bounds->bottom, region.bottom)}, replacement);
        }
    }
}

void RectanglePainter::paintRegion(const PixelBounds& bounds, const PixelState& replacement) {
    for (auto y = bounds.top; y < bounds.bottom; ++y) {
        for (auto x = bounds.left; x < bounds.right; ++x) {
            auto& visited = touched_[y * working_.width() + x];
            if (!visited || shouldReplace(working_.at(x, y), *replacement.winner)) {
                working_.setPixel(x, y, replacement);
            }
            visited = 1;
        }
    }
}

} // namespace compositor::detail
