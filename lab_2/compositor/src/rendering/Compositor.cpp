#include "rendering/Compositor.hpp"

#include "rendering/OverlayRule.hpp"
#include "rendering/RectangleClipper.hpp"

#include <cstdint>
#include <vector>

namespace compositor {

void Compositor::render(const Scene& scene, Image& canvas) const {
    const RectangleClipper clipper;
    const auto& rectangles = scene.rectangles();
    if (rectangles.empty()) {
        return;
    }
    Image working = canvas;
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
        for (std::size_t y = bounds->top; y < bounds->bottom; ++y) {
            for (std::size_t x = bounds->left; x < bounds->right; ++x) {
                auto& visited = touched[y * canvas.width() + x];
                if (!visited || shouldReplace(working.at(x, y), candidate)) {
                    working.setPixel(x, y, replacement);
                }
                visited = 1;
            }
        }
    }
    canvas.swap(working);
}

} // namespace compositor
