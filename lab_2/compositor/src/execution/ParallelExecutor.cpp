#include "execution/ParallelExecutor.hpp"
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
namespace compositor {
struct ParallelExecutor::Impl {
    std::mutex batchMutex;
    std::mutex mutex;
    std::condition_variable ready;
    std::condition_variable done;
    bool stopping = false;
    std::size_t generation = 0;
    std::size_t remaining = 0;
    std::size_t count = 0;
    std::atomic<std::size_t> next{0};
    Task task;
    std::exception_ptr error;
    // Destroyed before the state used by the workers.
    std::vector<std::jthread> workers;

    explicit Impl(std::size_t n) {
        if (n == 0) throw std::invalid_argument("Thread count must be positive");
        workers.reserve(n);
        try {
            for (std::size_t i = 0; i < n; ++i) workers.emplace_back([this] { work(); });
        } catch (...) {
            shutdown();
            throw;
        }
    }
    ~Impl() { shutdown(); }
    void shutdown() {
        { std::lock_guard lock(mutex); stopping = true; }
        ready.notify_all();
        workers.clear(); // jthread joins; state remains alive until all workers exit.
    }
    void work() {
        std::size_t seen = 0;
        for (;;) {
            std::unique_lock lock(mutex);
            ready.wait(lock, [&] { return stopping || generation != seen; });
            if (stopping) return;
            seen = generation;
            lock.unlock();
            for (;;) {
                auto index = next.load(std::memory_order_relaxed);
                // Bounded increment also handles counts near SIZE_MAX without wraparound.
                while (index < count && !next.compare_exchange_weak(
                           index, index + 1, std::memory_order_relaxed)) {}
                if (index >= count) break;
                try { task(index); }
                catch (...) {
                    std::lock_guard errorLock(mutex);
                    if (!error) error = std::current_exception();
                }
            }
            lock.lock();
            if (--remaining == 0) done.notify_one();
        }
    }
    void execute(std::size_t n, const Task& operation) {
        if (!operation) throw std::invalid_argument("Task must not be empty");
        std::lock_guard batchLock(batchMutex); // Serialize callers; recursive use is forbidden.
        if (n == 0) return;
        std::unique_lock lock(mutex);
        task = operation; // Can throw before publishing the batch.
        count = n;
        next.store(0, std::memory_order_relaxed);
        error = nullptr;
        remaining = workers.size();
        ++generation;
        ready.notify_all();
        done.wait(lock, [&] { return remaining == 0; });
        task = {};
        auto failure = error;
        error = nullptr;
        lock.unlock();
        if (failure) std::rethrow_exception(failure);
    }
};
ParallelExecutor::ParallelExecutor(std::size_t threadCount)
    : impl_(std::make_unique<Impl>(threadCount)) {}
ParallelExecutor::~ParallelExecutor() = default;
void ParallelExecutor::execute(std::size_t count, const Task& task) { impl_->execute(count, task); }
std::size_t ParallelExecutor::threadCount() const noexcept { return impl_->workers.size(); }
std::size_t ParallelExecutor::defaultThreadCount() noexcept {
    const auto count = std::thread::hardware_concurrency();
    return count == 0 ? 1 : count;
}
} // namespace compositor
