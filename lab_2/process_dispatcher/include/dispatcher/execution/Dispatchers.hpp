#pragma once
#include "dispatcher/execution/IDispatcher.hpp"
#include "dispatcher/logging/ILogger.hpp"
#include "dispatcher/output/IOutputStore.hpp"
namespace dispatcher {
class SyncDispatcher final : public IDispatcher {
  public:
    SyncDispatcher(IOutputStore &outputs, logging::ILogger &logger) : outputs_(outputs), logger_(logger) {}
    RunReport run(const ExecutionPlan &plan) override;

  private:
    IOutputStore &outputs_;
    logging::ILogger &logger_;
};
class AsyncDispatcher final : public IDispatcher {
  public:
    AsyncDispatcher(IOutputStore &outputs, logging::ILogger &logger) : outputs_(outputs), logger_(logger) {}
    RunReport run(const ExecutionPlan &plan) override;

  private:
    IOutputStore &outputs_;
    logging::ILogger &logger_;
};
} // namespace dispatcher
