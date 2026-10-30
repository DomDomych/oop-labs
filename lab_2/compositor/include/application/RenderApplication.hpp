#pragma once

#include "input/RunConfig.hpp"
#include "output/IImageWriter.hpp"

namespace compositor {

class RenderApplication {
public:
    // The borrowed writer must outlive the application.
    explicit RenderApplication(const IImageWriter& writer);
    // Creates the selected dependencies, renders, then saves exactly once.
    // Errors propagate to the caller. Unsafe mode retains its possible UB.
    void run(const RunConfig& config) const;

private:
    const IImageWriter& writer_;
};

} // namespace compositor
