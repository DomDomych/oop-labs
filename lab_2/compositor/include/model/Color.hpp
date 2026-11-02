#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace compositor {

inline constexpr std::size_t kRgbChannelCount = 3;
inline constexpr int kMinColorChannel = 0;
inline constexpr int kMaxColorChannel = std::numeric_limits<std::uint8_t>::max();

class Color {
public:
    Color(int red, int green, int blue);

    std::uint8_t red() const noexcept;
    std::uint8_t green() const noexcept;
    std::uint8_t blue() const noexcept;

    bool operator==(const Color&) const = default;

private:
    static std::uint8_t checkedChannel(int value);

    std::uint8_t red_;
    std::uint8_t green_;
    std::uint8_t blue_;
};

} // namespace compositor
