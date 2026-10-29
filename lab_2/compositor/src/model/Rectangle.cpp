#include "model/Rectangle.hpp"

#include <stdexcept>

namespace compositor {

Rectangle::Rectangle(int x, int y, int width, int height, Color color, int priority)
    : x_(x), y_(y), width_(width), height_(height), color_(color),
      priority_(priority) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Rectangle dimensions must be positive");
    }
}

int Rectangle::x() const noexcept { return x_; }

int Rectangle::y() const noexcept { return y_; }

int Rectangle::width() const noexcept { return width_; }

int Rectangle::height() const noexcept { return height_; }

Color Rectangle::color() const noexcept { return color_; }

int Rectangle::priority() const noexcept { return priority_; }

std::int64_t Rectangle::right() const noexcept {
    return static_cast<std::int64_t>(x_) + width_;
}

std::int64_t Rectangle::bottom() const noexcept {
    return static_cast<std::int64_t>(y_) + height_;
}

} // namespace compositor
