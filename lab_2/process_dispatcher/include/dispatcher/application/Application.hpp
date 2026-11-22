#pragma once
#include "dispatcher/execution/IDispatcher.hpp"
#include "dispatcher/logging/ILogger.hpp"
#include "dispatcher/output/IReportWriter.hpp"
namespace dispatcher {
// High-level use case depends only on contracts; composition belongs in main().
class Application {
  public:
    Application(IDispatcher &dispatcher, IReportWriter &report, logging::ILogger &logger)
        : dispatcher_(dispatcher), report_(report), logger_(logger) {}
    int run(const ExecutionPlan &plan);

  private:
    IDispatcher &dispatcher_;
    IReportWriter &report_;
    logging::ILogger &logger_;
};
} // namespace dispatcher
