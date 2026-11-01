#include "benchmark/ImageComparator.hpp"
#include <stdexcept>

namespace compositor::benchmark {

std::size_t countMismatchedPixels(const Image& expected, const Image& actual) {
    if (expected.width() != actual.width() || expected.height() != actual.height())
        throw std::invalid_argument("Cannot compare images of different dimensions");
    std::size_t count = 0;
    for (std::size_t y = 0; y < expected.height(); ++y)
        for (std::size_t x = 0; x < expected.width(); ++x)
            if (expected.at(x,y) != actual.at(x,y)) ++count;
    return count;
}

} // namespace compositor::benchmark
