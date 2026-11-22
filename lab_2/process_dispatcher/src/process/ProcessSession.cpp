#include "process/ProcessSession.hpp"
#include <boost/process/v2/stdio.hpp>
namespace dispatcher {
ProcessSession::ProcessSession(boost::asio::io_context &context, const Job &job)
    : output_(context), error_(context),
      child_(context, boost::filesystem::path(job.executable().native()), job.arguments(),
             boost::process::v2::process_stdio{nullptr, output_, error_}) {}
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
