#include "application/RenderComponents.hpp"
#include "execution/SequentialExecutor.hpp"
#include "execution/ParallelExecutor.hpp"
#include "synchronization/NoLockPolicy.hpp"
#include "synchronization/RegionMutexPolicy.hpp"
#include <stdexcept>

namespace compositor {

RenderComponents createRenderComponents(const RenderConfig& config) {
    if (config.threads == 0 || config.regionWidth == 0 || config.regionHeight == 0)
        throw std::invalid_argument("Thread count and region dimensions must be positive");
    switch (config.mode) {
    case RenderMode::Sequential:
        return {std::make_unique<SequentialExecutor>(), std::make_unique<NoLockPolicy>()};
    case RenderMode::ParallelUnsafe:
        return {std::make_unique<ParallelExecutor>(config.threads), std::make_unique<NoLockPolicy>()};
    case RenderMode::ParallelSafe:
        return {std::make_unique<ParallelExecutor>(config.threads), std::make_unique<RegionMutexPolicy>()};
    }
    throw std::invalid_argument("Unknown render mode");
}

} // namespace compositor
