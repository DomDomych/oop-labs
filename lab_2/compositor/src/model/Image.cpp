#include "model/Image.hpp"

#include <stdexcept>

namespace compositor {

std::size_t Image::width() const noexcept { return width_; }

std::size_t Image::height() const noexcept { return height_; }

Image::Image(std::size_t width, std::size_t height, Color initialColor)
    : width_(width), height_(height) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("Image dimensions must be positive");
    }
    if (width > pixels_.max_size() / height) {
        throw std::length_error("Image dimensions exceed storage capacity");
    }
    pixels_.assign(width * height, PixelState{initialColor});
}

std::size_t Image::offset(std::size_t x, std::size_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("Pixel coordinates are outside the image");
    }
    return y * width_ + x;
}

const PixelState& Image::at(std::size_t x, std::size_t y) const {
    return pixels_[offset(x, y)];
}

void Image::setPixel(std::size_t x, std::size_t y, const PixelState& state) {
    pixels_[offset(x, y)] = state;
}

Color Image::colorAt(std::size_t x, std::size_t y) const {
    return at(x, y).color;
}

} // namespace compositor
