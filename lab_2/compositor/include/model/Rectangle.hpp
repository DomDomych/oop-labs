#pragma once

#include "model/Color.hpp"

#include <cstdint>
#include <stdexcept>

namespace compositor {

class Rectangle {
public:
    Rectangle(int x, int y, int width, int height, Color color, int priority)
        : x_(x), y_(y), width_(width), height_(height), color_(color),
          priority_(priority) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Rectangle dimensions must be positive");
        }
    }

    int x() const noexcept { return x_; }
    int y() const noexcept { return y_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    Color color() const noexcept { return color_; }
    int priority() const noexcept { return priority_; }

    // Exclusive boundaries; widen before addition to avoid int overflow.
    std::int64_t right() const noexcept {
        return static_cast<std::int64_t>(x_) + width_;
    }
    std::int64_t bottom() const noexcept {
        return static_cast<std::int64_t>(y_) + height_;
    }

    bool operator==(const Rectangle&) const = default;

private:
    int x_;
    int y_;
    int width_;
    int height_;
    Color color_;
    int priority_;
};

} // namespace compositor
