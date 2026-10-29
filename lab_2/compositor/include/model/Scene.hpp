#pragma once

#include "model/Rectangle.hpp"

#include <vector>

namespace compositor {

// Input description. The vector position is the rectangle's tie-breaking index.
class Scene {
public:
    explicit Scene(std::vector<Rectangle> rectangles);

    const std::vector<Rectangle>& rectangles() const noexcept;

private:
    std::vector<Rectangle> rectangles_;
};

} // namespace compositor
