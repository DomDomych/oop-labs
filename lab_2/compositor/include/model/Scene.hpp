#pragma once

#include "model/Rectangle.hpp"

#include <cstddef>
#include <vector>

namespace compositor {

// Input description. The vector position is the rectangle's tie-breaking index.
class Scene {
public:
    Scene(std::size_t width, std::size_t height, Color background,
          std::vector<Rectangle> rectangles);

    std::size_t width() const noexcept;
    std::size_t height() const noexcept;
    Color background() const noexcept;
    const std::vector<Rectangle>& rectangles() const noexcept;

private:
    std::size_t width_;
    std::size_t height_;
    Color background_;
    std::vector<Rectangle> rectangles_;
};

} // namespace compositor
