#pragma once

#include "output/IImageWriter.hpp"

namespace compositor {

class PpmWriter final : public IImageWriter {
public:
    // Binary P6, RGB bytes in row-major order, maximum channel value 255.
    // Empty moved-from images are rejected. No durability guarantee on power loss.
    void write(const Image& image, const std::filesystem::path& path) const override;
};

} // namespace compositor
