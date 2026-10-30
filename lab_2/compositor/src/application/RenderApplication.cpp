#include "application/RenderApplication.hpp"

#include "execution/ParallelExecutor.hpp"
#include "execution/SequentialExecutor.hpp"
#include "rendering/Compositor.hpp"
#include "synchronization/NoLockPolicy.hpp"
#include "synchronization/RegionMutexPolicy.hpp"

#include <memory>
#include <stdexcept>

namespace compositor {

RenderApplication::RenderApplication(const IImageWriter& writer) : writer_(writer) {}

void RenderApplication::run(const RunConfig& config) const {
    if (config.render.threads == 0 || config.render.regionWidth == 0 || config.render.regionHeight == 0) {
        throw std::invalid_argument("Thread count and region dimensions must be positive");
    }
    std::unique_ptr<IExecutor> executor;
    std::unique_ptr<IRegionAccessPolicy> policy;
    switch (config.render.mode) {
    case RenderMode::Sequential:
        executor = std::make_unique<SequentialExecutor>();
        policy = std::make_unique<NoLockPolicy>();
        break;
    case RenderMode::ParallelUnsafe:
        executor = std::make_unique<ParallelExecutor>(config.render.threads);
        policy = std::make_unique<NoLockPolicy>();
        break;
    case RenderMode::ParallelSafe:
        executor = std::make_unique<ParallelExecutor>(config.render.threads);
        policy = std::make_unique<RegionMutexPolicy>();
        break;
    default:
        throw std::invalid_argument("Unknown render mode");
    }
    Image canvas{config.canvas.width, config.canvas.height, config.canvas.background};
    const Compositor compositor{config.render.regionWidth, config.render.regionHeight, *executor, *policy};
    compositor.render(config.scene, canvas);
    writer_.write(canvas, config.outputImage);
}

} // namespace compositor
