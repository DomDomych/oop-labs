#pragma once

#include "model/Image.hpp"
#include "model/Rectangle.hpp"
#include "rendering/RegionGrid.hpp"
#include "synchronization/IRegionAccessPolicy.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace compositor::detail {

// Internal state for one render call. The working image must outlive the painter
// and must not be moved or swapped while the painter is in use.
class RectanglePainter {
public:
    RectanglePainter(Image& working, std::size_t regionWidth, std::size_t regionHeight,
                     const IRegionAccessPolicy& accessPolicy);
    RectanglePainter(const RectanglePainter&) = delete;
    RectanglePainter& operator=(const RectanglePainter&) = delete;

    // Applies one rectangle with its position in the current scene.
    // Concurrent calls require an access policy that serializes each region.
    // Neither commits a transaction nor starts worker threads.
    void paint(const Rectangle& rectangle, std::size_t index);

private:
    void paintRegion(const PixelBounds& bounds, const PixelState& replacement);

    Image& working_;
    RegionGrid grid_;
    std::unique_ptr<IRegionAccess> access_;
    std::vector<std::uint8_t> touched_;
};

} // namespace compositor::detail
