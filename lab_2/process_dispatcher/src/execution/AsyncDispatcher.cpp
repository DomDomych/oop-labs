#include "dispatcher/execution/Dispatchers.hpp"
#include "execution/RunSession.hpp"
#include "platform/PipeErrors.hpp"
#include "process/ProcessSession.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <exception>

namespace dispatcher {
namespace {
namespace asio = boost::asio;
using asio::awaitable;
using asio::use_awaitable;

// Exactly one block is outstanding per stream. Disk writes run on a small pool;
// the event-loop thread remains available to service every other pipe/process.
awaitable<void> appendOnPool(asio::thread_pool &pool, IOutputCapture &capture, std::span<const char> bytes) {
    auto initiation = [&pool, &capture, bytes](auto handler) {
        auto executor = asio::get_associated_executor(handler);
        asio::post(pool, [&capture, bytes, executor, handler = std::move(handler)]() mutable {
            capture.append(bytes);
            asio::post(executor, [handler = std::move(handler)]() mutable { std::move(handler)(); });
        });
    };
    co_await asio::async_initiate<decltype(use_awaitable), void()>(initiation, use_awaitable);
}
awaitable<std::string> drain(asio::readable_pipe &pipe, IOutputCapture &capture, asio::thread_pool &pool,
                             std::size_t blockSize) {
    std::vector<char> buffer(blockSize);
    for (;;) {
        boost::system::error_code ec;
        const auto count =
            co_await pipe.async_read_some(asio::buffer(buffer), asio::redirect_error(use_awaitable, ec));
        if (count)
            co_await appendOnPool(pool, capture, {buffer.data(), count});
        if (ec) {
            boost::system::error_code ignored;
            pipe.close(ignored);
            co_return pipeEnded(ec) ? std::string{} : ec.message();
        }
    }
}
struct WaitResult {
    std::optional<int> code;
    std::string error;
};
awaitable<WaitResult> wait(ProcessSession &process, RunSession &run, std::size_t index) {
    boost::system::error_code ec;
    const int code = co_await process.child().async_wait(asio::redirect_error(use_awaitable, ec));
    WaitResult result{ec ? std::nullopt : std::optional<int>(code), ec ? ec.message() : ""};
    // Record immediately, before awaiting EOF or file writes on either output stream.
    run.exited(index, result.code, ec ? "wait_failed" : "process_exited");
    co_return result;
}
awaitable<void> execute(std::size_t index, const ExecutionPlan &plan, asio::io_context &context,
                        asio::thread_pool &pool, IOutputStore &store, RunSession &run) {
    using namespace asio::experimental::awaitable_operators;
    const auto &job = plan.jobs()[index];
    JobResult result;
    result.id = job.id();
    result.startedAt = WallClock::now();
    const auto start = SteadyClock::now();
    OutputPair outputs;
    bool launched = false;
    try {
        outputs = store.open(job);
        ProcessSession process(context, job);
        launched = true;
        run.started(index);
        auto [outError, errError, waited] =
            co_await (drain(process.output(), *outputs.standardOutput, pool, plan.blockSize()) &&
                      drain(process.error(), *outputs.standardError, pool, plan.blockSize()) &&
                      wait(process, run, index));
        result.exitCode = waited.code;
        result.error = std::move(waited.error);
        result.status = JobStatus::WaitFailed;
        result.standardOutput = outputs.standardOutput->finish();
        result.standardError = outputs.standardError->finish();
        if (!outError.empty())
            result.standardOutput.error += " read: " + outError;
        if (!errError.empty())
            result.standardError.error += " read: " + errError;
        finalizeStatus(result);
    } catch (const std::exception &error) {
        // Normal OS launch errors are local to this job. Unexpected exceptions after
        // launch propagate: inventing a second exit notification would corrupt the journal.
        if (launched)
            throw;
        result.status = outputs.standardOutput ? JobStatus::LaunchFailed : JobStatus::IoFailed;
        result.error = error.what();
        run.exited(index, {}, outputs.standardOutput ? "launch_failed" : "output_failed");
        if (outputs.standardOutput)
            result.standardOutput = outputs.standardOutput->finish();
        if (outputs.standardError)
            result.standardError = outputs.standardError->finish();
    }
    result.finishedAt = WallClock::now();
    result.duration = SteadyClock::now() - start;
    run.finished(index, std::move(result));
}
awaitable<void> worker(std::size_t &next, const ExecutionPlan &plan, asio::io_context &context,
                       asio::thread_pool &pool, IOutputStore &store, RunSession &run) {
    while (next < plan.jobs().size()) {
        const auto index = next++; // All workers resume on the same event-loop thread.
        co_await execute(index, plan, context, pool, store, run);
    }
}
} // namespace
RunReport AsyncDispatcher::run(const ExecutionPlan &plan) {
    RunSession run(plan, "async", logger_);
    asio::io_context context;
    asio::thread_pool writers(2);
    std::size_t next = 0;
    std::exception_ptr failure;
    // Guard also covers intervals when all pending work is in the file writer pool.
    auto guard = asio::make_work_guard(context);
    std::size_t workers = std::min(plan.concurrency(), plan.jobs().size());
    if (workers == 0)
        guard.reset();
    for (std::size_t i = 0, count = workers; i < count; ++i)
        asio::co_spawn(context, worker(next, plan, context, writers, outputs_, run),
                       [&](std::exception_ptr error) {
                           if (error && !failure)
                               failure = error;
                           if (--workers == 0)
                               guard.reset();
                       });
    context.run();
    writers.join();
    if (failure)
        std::rethrow_exception(failure);
    return run.finish();
}
} // namespace dispatcher
