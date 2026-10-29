#include "execution/ParallelExecutor.hpp"
#include "execution/SequentialExecutor.hpp"
#include "rendering/Compositor.hpp"
#include "synchronization/NoLockPolicy.hpp"
#include "synchronization/RegionMutexPolicy.hpp"
#include <atomic>
#include <barrier>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace compositor;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void same(const Image& a, const Image& b) {
    require(a.width() == b.width() && a.height() == b.height(), "Dimensions differ");
    for (std::size_t y=0; y<a.height(); ++y)
        for (std::size_t x=0; x<a.width(); ++x)
            require(a.at(x,y)==b.at(x,y), "Pixel or metadata differs");
}
void batchContract(IExecutor& executor) {
    for (int round=0; round<10; ++round) {
        std::vector<std::atomic<int>> counts(97);
        std::atomic<int> active{0};
        bool failed=false;
        try {
            executor.execute(counts.size(), [&](std::size_t i) {
                ++active;
                ++counts[i];
                --active;
                if (round%2 == 0 && (i==4 || i==31)) throw std::runtime_error("task failed");
            });
        } catch (const std::runtime_error&) { failed=true; }
        require(failed == (round%2==0), "Failure not propagated");
        require(active==0, "Returned before all work completed");
        for (auto& count:counts) require(count==1, "Task skipped or duplicated");
    }
    executor.execute(0, [](std::size_t) { throw std::runtime_error("Empty batch executed"); });
    bool rejected=false;
    try { executor.execute(1, {}); } catch (const std::invalid_argument&) { rejected=true; }
    require(rejected, "Empty callable accepted");
}
class FailAfterWrite final : public IRegionAccessPolicy {
    class Access final : public IRegionAccess {
        std::unique_ptr<IRegionAccess> locks_;
        std::atomic<bool> thrown_{false};
    public:
        explicit Access(std::size_t count) : locks_(RegionMutexPolicy{}.create(count)) {}
        void withRegion(std::size_t id, const std::function<void()>& operation) override {
            locks_->withRegion(id, [&] {
                operation();
                if (!thrown_.exchange(true)) throw std::runtime_error("Failure after writing");
            });
        }
    };
public:
    std::unique_ptr<IRegionAccess> create(std::size_t count) const override {
        return std::make_unique<Access>(count);
    }
};
void rendering() {
    const Color white{255,255,255};
    std::vector<Rectangle> rectangles;
    for (int i=0; i<100; ++i)
        rectangles.emplace_back(i%13-4, i%11-3, 11+i%5, 7+i%3,
                                Color{i,255-i,i*2}, i%7-3);
    const Scene scene{rectangles};
    const NoLockPolicy unlocked;
    const RegionMutexPolicy locked;
    SequentialExecutor sequential;
    Image initial{23,17,white};
    initial.setPixel(0,0,PixelState{Color{1,2,3},RectangleMetadata{999,8}});
    Image expected{initial};
    Compositor{4,3,sequential,unlocked}.render(scene,expected);
    for (std::size_t threads : {1u,2u,4u}) {
        ParallelExecutor pool{threads};
        for (std::size_t region : {1u,3u,32u}) {
            Compositor compositor{region,region,pool,locked};
            Image actual{initial};
            compositor.render(scene,actual);
            same(actual,expected);
            compositor.render(scene,actual);
            same(actual,expected);
            const Scene overlay{{Rectangle{0,0,2,2,Color{9,8,7},-100}}};
            Image overlayExpected{expected};
            Compositor{region,region,sequential,unlocked}.render(overlay,overlayExpected);
            compositor.render(overlay,actual);
            same(actual,overlayExpected);
        }
        const FailAfterWrite failure;
        Image unchanged{initial};
        bool caught=false;
        try { Compositor{2,2,pool,failure}.render(scene,unchanged); }
        catch (const std::runtime_error&) { caught=true; }
        require(caught,"Missing injected error");
        same(unchanged,initial);
        Compositor{2,2,pool,locked}.render(scene,unchanged);
        same(unchanged,expected);
    }
    // Exercise unlocked parallel configuration without inducing a data race.
    ParallelExecutor pool{2};
    const Scene disjoint{{Rectangle{0,0,1,1,Color{1,2,3},0},
                          Rectangle{2,2,1,1,Color{3,2,1},0}}};
    Image a{initial}, b{initial};
    Compositor{1,1,sequential,unlocked}.render(disjoint,a);
    Compositor{1,1,pool,unlocked}.render(disjoint,b);
    same(a,b);
}
void policies() {
    RegionMutexPolicy policy;
    auto access=policy.create(2);
    ParallelExecutor pool{4};
    int count=0;
    pool.execute(200,[&](std::size_t) { access->withRegion(0,[&] { ++count; }); });
    require(count==200,"Region is not exclusive");
    try { access->withRegion(0,[] { throw std::runtime_error("failure"); }); }
    catch (const std::runtime_error&) {}
    access->withRegion(0,[&] { ++count; });
    require(count==201,"Lock not released after exception");
    bool rejected=false;
    try { access->withRegion(2,[] {}); } catch (const std::out_of_range&) { rejected=true; }
    require(rejected,"Invalid region accepted");
}
}
int main() {
    try {
        bool rejected=false;
        try { ParallelExecutor invalid{0}; } catch (const std::invalid_argument&) { rejected=true; }
        require(rejected,"Zero workers accepted");
        require(ParallelExecutor::defaultThreadCount()>0,"Invalid default count");
        SequentialExecutor sequential;
        batchContract(sequential);
        ParallelExecutor pool{4};
        require(pool.threadCount()==4,"Incorrect worker count");
        std::barrier rendezvous{4};
        pool.execute(4,[&](std::size_t) { rendezvous.arrive_and_wait(); });
        batchContract(pool);
        { ParallelExecutor idle{2}; }
        policies();
        rendering();
        return 0;
    } catch (const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
