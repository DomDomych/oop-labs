#pragma once

#include "model/Image.hpp"

namespace compositor::benchmark {

// Counts differing complete PixelState values (color AND winner metadata).
// Dimension mismatch is an error, not a zero mismatch result.
std::size_t countMismatchedPixels(const Image& expected, const Image& actual);

} // namespace compositor::benchmark
