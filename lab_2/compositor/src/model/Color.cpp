#include "model/Color.hpp"

#include <stdexcept>
#include <string>

namespace compositor {

Color::Color(int red, int green, int blue)
    : red_(checkedChannel(red)), green_(checkedChannel(green)),
      blue_(checkedChannel(blue)) {}

std::uint8_t Color::red() const noexcept { return red_; }

std::uint8_t Color::green() const noexcept { return green_; }

std::uint8_t Color::blue() const noexcept { return blue_; }

std::uint8_t Color::checkedChannel(int value) {
    if (value < kMinColorChannel || value > kMaxColorChannel) {
        throw std::invalid_argument("RGB channel must be in [" + std::to_string(kMinColorChannel) + ", " +
                                    std::to_string(kMaxColorChannel) + "]");
    }
    return static_cast<std::uint8_t>(value);
}

} // namespace compositor
