#pragma once

#include "input/RunConfig.hpp"

#include <filesystem>

namespace compositor {

class JsonConfigLoader {
public:
    // Reads and validates the complete configuration before returning.
    // All schema fields are required; unknown fields are rejected.
    // Relative output paths are resolved against the configuration directory.
    // Input/validation errors throw std::runtime_error with file/field context.
    // Does not create the output file or start rendering.
    RunConfig load(const std::filesystem::path& path) const;
};

} // namespace compositor
