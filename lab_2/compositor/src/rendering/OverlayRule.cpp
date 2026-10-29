#include "rendering/OverlayRule.hpp"

namespace compositor {

bool shouldReplace(const PixelState& current,
                   const RectangleMetadata& candidate) noexcept {
    return !current.winner || candidate.priority > current.winner->priority ||
           (candidate.priority == current.winner->priority &&
            candidate.index > current.winner->index);
}

} // namespace compositor
