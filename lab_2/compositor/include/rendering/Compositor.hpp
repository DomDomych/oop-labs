#pragma once

#include "model/Image.hpp"
#include "model/Scene.hpp"

namespace compositor {

class Compositor {
public:
    // Builds a new image sequentially without changing the scene.
    // Allocation errors propagate to the caller.
    Image render(const Scene& scene) const;
};

} // namespace compositor
