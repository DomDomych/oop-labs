#pragma once

#include "model/PixelState.hpp"

#include <cstddef>
#include <vector>

namespace compositor {

// Fixed-size storage. Synchronization belongs to the caller, not this model.
class Image {
public:
    Image(std::size_t width, std::size_t height, Color background);

    std::size_t width() const noexcept { return width_; }
    std::size_t height() const noexcept { return height_; }

    const PixelState& at(std::size_t x, std::size_t y) const;
    void setPixel(std::size_t x, std::size_t y, const PixelState& state);
    Color colorAt(std::size_t x, std::size_t y) const;

private:
    std::size_t offset(std::size_t x, std::size_t y) const;

    std::size_t width_;
    std::size_t height_;
    std::vector<PixelState> pixels_;
};

} // namespace compositor
