#include "synchronization/NoLockPolicy.hpp"
#include <stdexcept>
namespace compositor {
namespace {
class NoLockAccess final : public IRegionAccess {
public:
    explicit NoLockAccess(std::size_t count) : count_(count) {}
    void withRegion(std::size_t region, const std::function<void()>& operation) override {
        if (region >= count_) throw std::out_of_range("Region index is outside access policy");
        operation();
    }
private:
    std::size_t count_;
};
}
std::unique_ptr<IRegionAccess> NoLockPolicy::create(std::size_t regionCount) const {
    return std::make_unique<NoLockAccess>(regionCount);
}
} // namespace compositor
