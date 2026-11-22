#include "platform/PipeDrainer.hpp"
#include <cerrno>
#include <poll.h>
#include <system_error>
namespace dispatcher {
// Blocking multiplexing is needed even with one child: reading stdout to EOF before
// stderr deadlocks when the child fills its stderr pipe. No async APIs or reader threads.
void drainBlocking(ProcessSession &process, OutputPair &outputs, std::size_t blockSize,
                   std::array<std::string, 2> &errors) {
    std::array<boost::asio::readable_pipe *, 2> pipes{&process.output(), &process.error()};
    std::array<IOutputCapture *, 2> captures{outputs.standardOutput.get(), outputs.standardError.get()};
    std::array<pollfd, 2> fds{
        {{pipes[0]->native_handle(), POLLIN, 0}, {pipes[1]->native_handle(), POLLIN, 0}}};
    std::vector<char> buffer(blockSize);
    while (fds[0].fd >= 0 || fds[1].fd >= 0) {
        int ready;
        do {
            ready = ::poll(fds.data(), fds.size(), -1);
        } while (ready < 0 && errno == EINTR);
        if (ready < 0)
            throw std::system_error(errno, std::generic_category(), "poll child pipes");
        for (std::size_t i = 0; i < 2; ++i) {
            if (fds[i].fd < 0 || fds[i].revents == 0)
                continue;
            boost::system::error_code ec;
            const auto bytes = pipes[i]->read_some(boost::asio::buffer(buffer), ec);
            if (bytes)
                captures[i]->append({buffer.data(), bytes});
            if (ec) {
                if (ec != boost::asio::error::eof)
                    errors[i] = ec.message();
                pipes[i]->close();
                fds[i].fd = -1;
            }
        }
    }
}
} // namespace dispatcher
