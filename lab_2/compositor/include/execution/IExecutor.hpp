#pragma once
#include <cstddef>
#include <functional>
namespace compositor {
class IExecutor {
public:
    using Task = std::function<void(std::size_t)>;
    virtual ~IExecutor() = default;
    // May invoke task concurrently, without ordering guarantees. Each index is
    // attempted once; task exceptions do not cancel the remaining indices.
    // Returns/rethrows only after every invocation has finished. Does not retain task.
    // Empty tasks are invalid. Recursive use of the same executor is unsupported.
    virtual void execute(std::size_t count, const Task& task) = 0;
};
} // namespace compositor
