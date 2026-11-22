#include "dispatcher/execution/Dispatchers.hpp"
#include "execution/RunSession.hpp"
#include "platform/PipeDrainer.hpp"
namespace dispatcher {
RunReport SyncDispatcher::run(const ExecutionPlan &plan) {
    RunSession run(plan, "sync", logger_);
    boost::asio::io_context context;
    for (std::size_t index = 0; index < plan.jobs().size(); ++index) {
        const auto &job = plan.jobs()[index];
        JobResult result;
        result.id = job.id();
        result.startedAt = WallClock::now();
        const auto start = SteadyClock::now();
        OutputPair outputs;
        bool launched = false, notified = false;
        try {
            outputs = outputs_.open(job);
            ProcessSession process(context, job);
            launched = true;
            run.started(index);
            std::array<std::string, 2> errors;
            drainBlocking(process, outputs, plan.blockSize(), errors);
            boost::system::error_code ec;
            const int code = process.child().wait(ec);
            if (ec) {
                result.status = JobStatus::WaitFailed;
                result.error = ec.message();
            } else
                result.exitCode = code;
            run.exited(index, result.exitCode, ec ? "wait_failed" : "process_exited");
            notified = true;
            result.standardOutput = outputs.standardOutput->finish();
            result.standardError = outputs.standardError->finish();
            if (!errors[0].empty())
                result.standardOutput.error += " read: " + errors[0];
            if (!errors[1].empty())
                result.standardError.error += " read: " + errors[1];
            finalizeStatus(result);
        } catch (const std::exception &error) {
            result.status = launched
                                ? JobStatus::IoFailed
                                : (outputs.standardOutput ? JobStatus::LaunchFailed : JobStatus::IoFailed);
            result.error = error.what();
            if (!notified)
                run.exited(index, {},
                           launched ? "io_failed"
                                    : (outputs.standardOutput ? "launch_failed" : "output_failed"));
            if (outputs.standardOutput)
                result.standardOutput = outputs.standardOutput->finish();
            if (outputs.standardError)
                result.standardError = outputs.standardError->finish();
        }
        result.finishedAt = WallClock::now();
        result.duration = SteadyClock::now() - start;
        run.finished(index, std::move(result));
    }
    return run.finish();
}
} // namespace dispatcher
