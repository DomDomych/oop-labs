#pragma once

#include "model/Rectangle.hpp"

#include <cstddef>
#include <vector>

namespace compositor {

// Input description. The vector position is the rectangle's tie-breaking index.
// Canvas dimensions are validated when an Image is constructed.
struct Scene {
    std::size_t width;
    std::size_t height;
    Color background;
    std::vector<Rectangle> rectangles;
};

} // namespace compositor
