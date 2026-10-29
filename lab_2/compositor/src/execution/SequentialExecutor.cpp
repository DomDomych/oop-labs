#include "execution/SequentialExecutor.hpp"
#include <exception>
#include <stdexcept>
namespace compositor {
void SequentialExecutor::execute(std::size_t count, const Task& task) {
    if (!task) throw std::invalid_argument("Task must not be empty");
    std::exception_ptr error;
    for (std::size_t i = 0; i < count; ++i) {
        try { task(i); }
        catch (...) { if (!error) error = std::current_exception(); }
    }
    if (error) std::rethrow_exception(error);
}
} // namespace compositor
