#ifndef AW_TPL_THREADING_TASKS_TASK_ALREADY_MOVED_EXCEPTION_HPP
#define AW_TPL_THREADING_TASKS_TASK_ALREADY_MOVED_EXCEPTION_HPP

#include <exception>
#include <string>

namespace aw::tpl::threading::tasks {

/**
 * @class TaskAlreadyMovedException
 * @brief Thrown when attempting to extract a move-only result from a @c Task<T> more than once.
 */
class TaskAlreadyMovedException : public std::exception {
public:
    /// Creates the exception with a message explaining that the move-only result is gone.
    TaskAlreadyMovedException();

    /// Returns the exception message.
    [[nodiscard]] const char* what() const noexcept override;

    /**
     * @brief Returns the reason the move already occurred.
     * @return Reason string describing the context.
     */
    [[nodiscard]] const std::string& exception_reason() const noexcept { return reason_; }

private:
    std::string message_;
    std::string reason_;
};

} // namespace aw::tpl::threading::tasks

#endif  // AW_TPL_THREADING_TASKS_TASK_ALREADY_MOVED_EXCEPTION_HPP
