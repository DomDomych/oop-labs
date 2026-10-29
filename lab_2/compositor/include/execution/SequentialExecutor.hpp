#pragma once
#include "execution/IExecutor.hpp"
namespace compositor {
class SequentialExecutor final : public IExecutor {
public:
    void execute(std::size_t count, const Task& task) override;
};
} // namespace compositor
