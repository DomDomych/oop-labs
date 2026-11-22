#pragma once
#include "dispatcher/model/Model.hpp"
namespace dispatcher {
class IDispatcher {
  public:
    virtual ~IDispatcher() = default;
    // Returns after every job has a result and both output streams are drained.
    // Individual job failures are results, not exceptions. Infrastructure errors may throw.
    virtual RunReport run(const ExecutionPlan &plan) = 0;
};
} // namespace dispatcher
