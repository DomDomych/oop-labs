#include "model/Scene.hpp"

#include <stdexcept>
#include <utility>

namespace compositor {

Scene::Scene(std::size_t width, std::size_t height, Color background,
             std::vector<Rectangle> rectangles)
    : width_(width), height_(height), background_(background),
      rectangles_(std::move(rectangles)) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("Scene dimensions must be positive");
    }
}

std::size_t Scene::width() const noexcept { return width_; }

std::size_t Scene::height() const noexcept { return height_; }

Color Scene::background() const noexcept { return background_; }

const std::vector<Rectangle>& Scene::rectangles() const noexcept {
    return rectangles_;
}

} // namespace compositor
