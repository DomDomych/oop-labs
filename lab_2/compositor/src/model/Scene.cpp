#include "model/Scene.hpp"

#include <utility>

namespace compositor {

Scene::Scene(std::vector<Rectangle> rectangles)
    : rectangles_(std::move(rectangles)) {}

const std::vector<Rectangle>& Scene::rectangles() const noexcept {
    return rectangles_;
}

} // namespace compositor
