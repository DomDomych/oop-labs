#include "dispatcher/application/Application.hpp"
namespace dispatcher {
int Application::run(const ExecutionPlan &plan) {
    const auto start = SteadyClock::now();
    logger_.log({WallClock::now(), logging::Level::Info, "program_started", {}});
    int code = 2;
    try {
        const auto result = dispatcher_.run(plan);
        report_.write(result);
        code = result.successful() ? 0 : 3;
    } catch (const std::exception &error) {
        logger_.log(
            {WallClock::now(),
             logging::Level::Error,
             "program_finished",
             {{"duration_ns",
               std::to_string(
                   std::chrono::duration_cast<std::chrono::nanoseconds>(SteadyClock::now() - start).count())},
              {"exit_code", "2"},
              {"error", error.what()}}});
        throw;
    }
    logger_.log(
        {WallClock::now(),
         logging::Level::Info,
         "program_finished",
         {{"duration_ns",
           std::to_string(
               std::chrono::duration_cast<std::chrono::nanoseconds>(SteadyClock::now() - start).count())},
          {"exit_code", std::to_string(code)}}});
    return code;
}
} // namespace dispatcher
