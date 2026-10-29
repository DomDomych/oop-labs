#pragma once
#include <cstddef>
#include <functional>
#include <memory>
namespace compositor {
// A call-scoped access mechanism, not an unconditional thread-safety guarantee.
class IRegionAccess {
public:
    virtual ~IRegionAccess() = default;
    // Executes synchronously exactly once if acquisition succeeds; propagates errors.
    // The operation must not call withRegion on this same access object, directly
    // or indirectly, for any region. Nested acquisition is not supported.
    virtual void withRegion(std::size_t region, const std::function<void()>& operation) = 0;
};
class IRegionAccessPolicy {
public:
    virtual ~IRegionAccessPolicy() = default;
    // Fresh region state for each render call, valid for IDs [0, regionCount).
    // Returns a non-null access object or throws; nullptr is not a valid result.
    virtual std::unique_ptr<IRegionAccess> create(std::size_t regionCount) const = 0;
};
} // namespace compositor
