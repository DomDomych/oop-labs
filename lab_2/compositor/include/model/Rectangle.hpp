#pragma once

#include "model/Color.hpp"

#include <cstdint>

namespace compositor {

class Rectangle {
public:
    Rectangle(int x, int y, int width, int height, Color color, int priority);

    int x() const noexcept;
    int y() const noexcept;
    int width() const noexcept;
    int height() const noexcept;
    Color color() const noexcept;
    int priority() const noexcept;

    // Exclusive boundaries; widen before addition to avoid int overflow.
    std::int64_t right() const noexcept;
    std::int64_t bottom() const noexcept;

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
