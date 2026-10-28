#pragma once

#include <cstdint>
#include <stdexcept>

namespace compositor {

class Color {
public:
    constexpr Color(int red, int green, int blue)
        : red_(checkedChannel(red)), green_(checkedChannel(green)),
          blue_(checkedChannel(blue)) {}

    constexpr std::uint8_t red() const noexcept { return red_; }
    constexpr std::uint8_t green() const noexcept { return green_; }
    constexpr std::uint8_t blue() const noexcept { return blue_; }

    bool operator==(const Color&) const = default;

private:
    static constexpr std::uint8_t checkedChannel(int value) {
        if (value < 0 || value > 255) {
            throw std::invalid_argument("RGB channel must be in [0, 255]");
        }
        return static_cast<std::uint8_t>(value);
    }

    std::uint8_t red_;
    std::uint8_t green_;
    std::uint8_t blue_;
};

} // namespace compositor
