#pragma once

#include "model/Image.hpp"
#include <filesystem>

namespace compositor {

class IImageWriter {
public:
    virtual ~IImageWriter() = default;

    // Saves color data without changing the image. Throws on failure.
    // Replaces an existing file only after successful staging of the complete
    // output. On failure the previous destination is preserved.
    // The destination parent directory must already exist.
    virtual void write(const Image& image, const std::filesystem::path& path) const = 0;
};

} // namespace compositor
