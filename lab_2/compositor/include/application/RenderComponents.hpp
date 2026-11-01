#pragma once

#include "execution/IExecutor.hpp"
#include "synchronization/IRegionAccessPolicy.hpp"
#include "input/RunConfig.hpp"
#include <memory>

namespace compositor {

struct RenderComponents {
    std::unique_ptr<IExecutor> executor;
    std::unique_ptr<IRegionAccessPolicy> policy;
};

// Shared composition root for application and benchmark mode selection.
RenderComponents createRenderComponents(const RenderConfig& config);

} // namespace compositor
