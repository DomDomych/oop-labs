#pragma once

#include "model/PixelState.hpp"

#include <cstddef>
#include <vector>

namespace compositor {

// Fixed-size storage. Synchronization belongs to the caller, not this model.
class Image {
public:
    Image(std::size_t width, std::size_t height, Color initialColor);
    Image(const Image&) = default;
    // The source becomes empty (0 x 0); pixel access then throws out_of_range.
    Image(Image&& other) noexcept;
    Image& operator=(const Image&) = delete;
    Image& operator=(Image&&) = delete;

    std::size_t width() const noexcept;
    std::size_t height() const noexcept;

    const PixelState& at(std::size_t x, std::size_t y) const;
    void setPixel(std::size_t x, std::size_t y, const PixelState& state);
    Color colorAt(std::size_t x, std::size_t y) const;

    // Exchanges complete storage without allocation; commits a prepared result.
    void swap(Image& other) noexcept;

private:
    std::size_t offset(std::size_t x, std::size_t y) const;

    std::size_t width_;
    std::size_t height_;
    std::vector<PixelState> pixels_;
};

} // namespace compositor
