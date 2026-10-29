#include "rendering/Compositor.hpp"

#include "RectanglePainter.hpp"

#include <stdexcept>

namespace compositor {

Compositor::Compositor(std::size_t regionWidth, std::size_t regionHeight,
                       IExecutor& executor, const IRegionAccessPolicy& accessPolicy)
    : executor_(executor), accessPolicy_(accessPolicy),
      regionWidth_(regionWidth), regionHeight_(regionHeight) {
    if (regionWidth == 0 || regionHeight == 0) {
        throw std::invalid_argument("Region dimensions must be positive");
    }
}

void Compositor::render(const Scene& scene, Image& canvas) const {
    if (canvas.width() == 0 || canvas.height() == 0) {
        throw std::invalid_argument("Cannot render onto an empty moved-from canvas");
    }
    const auto& rectangles = scene.rectangles();
    if (rectangles.empty()) {
        return;
    }
    Image working = canvas;
    {
        detail::RectanglePainter painter{working, regionWidth_, regionHeight_, accessPolicy_};
        executor_.execute(rectangles.size(), [&](std::size_t index) {
            painter.paint(rectangles.at(index), index);
        });
    }
    canvas.swap(working);
}

} // namespace compositor
