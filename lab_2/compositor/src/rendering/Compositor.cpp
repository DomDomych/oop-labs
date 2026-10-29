#include "rendering/Compositor.hpp"

#include "rendering/OverlayRule.hpp"
#include "rendering/RectangleClipper.hpp"

namespace compositor {

void Compositor::render(const Scene& scene, Image& canvas) const {
    const RectangleClipper clipper;
    const auto& rectangles = scene.rectangles();
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
                if (shouldReplace(canvas.at(x, y), candidate)) {
                    canvas.setPixel(x, y, replacement);
                }
            }
        }
    }
}

} // namespace compositor
