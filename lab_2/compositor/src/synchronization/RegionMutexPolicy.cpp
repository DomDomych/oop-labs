#include "synchronization/RegionMutexPolicy.hpp"
#include <mutex>
#include <stdexcept>
#include <vector>
namespace compositor {
namespace {
class MutexAccess final : public IRegionAccess {
public:
    explicit MutexAccess(std::size_t count) : mutexes_(count) {}
    void withRegion(std::size_t region, const std::function<void()>& operation) override {
        if (region >= mutexes_.size()) throw std::out_of_range("Region index is outside access policy");
        std::lock_guard lock(mutexes_[region]);
        operation();
    }
private:
    std::vector<std::mutex> mutexes_;
};
}
std::unique_ptr<IRegionAccess> RegionMutexPolicy::create(std::size_t regionCount) const {
    return std::make_unique<MutexAccess>(regionCount);
}
} // namespace compositor
