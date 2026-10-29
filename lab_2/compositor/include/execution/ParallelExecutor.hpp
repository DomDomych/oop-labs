#pragma once
#include "execution/IExecutor.hpp"
#include <memory>
namespace compositor {
class ParallelExecutor final : public IExecutor {
public:
    // Zero is invalid. Default uses hardware_concurrency, falling back to one.
    explicit ParallelExecutor(std::size_t threadCount = defaultThreadCount());
    ~ParallelExecutor() override;
    ParallelExecutor(const ParallelExecutor&) = delete;
    ParallelExecutor& operator=(const ParallelExecutor&) = delete;
    void execute(std::size_t count, const Task& task) override;
    std::size_t threadCount() const noexcept;
    static std::size_t defaultThreadCount() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace compositor
