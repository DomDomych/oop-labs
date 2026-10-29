#pragma once
#include "synchronization/IRegionAccessPolicy.hpp"
namespace compositor {
class NoLockPolicy final : public IRegionAccessPolicy {
public:
    std::unique_ptr<IRegionAccess> create(std::size_t regionCount) const override;
};
} // namespace compositor
