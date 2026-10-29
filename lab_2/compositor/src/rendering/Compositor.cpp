#include "rendering/Compositor.hpp"

#include "rendering/OverlayRule.hpp"
#include "rendering/RectangleClipper.hpp"

namespace compositor {

Image Compositor::render(const Scene& scene) const {
    Image image{scene.width(), scene.height(), scene.background()};
    const RectangleClipper clipper;
    const auto& rectangles = scene.rectangles();
    for (std::size_t index = 0; index < rectangles.size(); ++index) {
        const auto& rectangle = rectangles[index];
        const auto bounds = clipper.clip(rectangle, image.width(), image.height());
        if (!bounds) {
            continue;
        }
        const RectangleMetadata candidate{rectangle.priority(), index};
        const PixelState replacement{rectangle.color(), candidate};
        for (std::size_t y = bounds->top; y < bounds->bottom; ++y) {
            for (std::size_t x = bounds->left; x < bounds->right; ++x) {
                if (shouldReplace(image.at(x, y), candidate)) {
                    image.setPixel(x, y, replacement);
                }
            }
        }
    }
    return image;
}

} // namespace compositor
