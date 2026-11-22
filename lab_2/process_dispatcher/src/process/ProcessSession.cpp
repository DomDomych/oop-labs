#include "process/ProcessSession.hpp"
#include "platform/Platform.hpp"
#include <boost/process/v2/stdio.hpp>
namespace dispatcher {
namespace {
boost::process::v2::process launch(boost::asio::io_context &context, const Job &job,
                                   boost::asio::readable_pipe &output, boost::asio::readable_pipe &error) {
    const boost::process::v2::filesystem::path path(job.executable().native());
#ifdef _WIN32
    const auto command = platform::commandLine(job.executable(), job.arguments());
    return {context, path, command.c_str(), boost::process::v2::process_stdio{nullptr, output, error}};
#else
    return {context, path, job.arguments(), boost::process::v2::process_stdio{nullptr, output, error}};
#endif
}
} // namespace
ProcessSession::ProcessSession(boost::asio::io_context &context, const Job &job)
    : output_(context), error_(context), child_(launch(context, job, output_, error_)) {}
ProcessSession::~ProcessSession() {
    // Exceptional unwinding must also reap the child, not leave a zombie.
    boost::system::error_code ec;
    if (child_.is_open()) {
        if (child_.running(ec))
            child_.terminate(ec);
        child_.wait(ec);
    }
}
} // namespace dispatcher
