#pragma once
#include "dispatcher/logging/ILogger.hpp"
#include "dispatcher/model/Model.hpp"
#include <optional>

namespace dispatcher {
// Single-threaded collector shared by both dispatchers. Asio callbacks execute on one io_context thread.
class RunSession {
  public:
    RunSession(const ExecutionPlan &plan, std::string mode, logging::ILogger &logger);
    void started(std::size_t index);
    void exited(std::size_t index, std::optional<int> code, std::string kind);
    void finished(std::size_t index, JobResult result);
    RunReport finish();

  private:
    const ExecutionPlan &plan_;
    logging::ILogger &logger_;
    RunReport report_;
    SteadyClock::time_point start_;
    std::size_t active_ = 0;
    std::vector<bool> started_, completed_, notified_;
};
void finalizeStatus(JobResult &result);
} // namespace dispatcher
