#pragma once

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
    const char* what() const noexcept override;

    /**
     * @brief Returns the reason the move already occurred.
     * @return Reason string describing the context.
     */
    const std::string& exception_reason() const noexcept { return reason_; }

private:
    std::string message_;
    std::string reason_;
};

} // namespace aw::tpl::threading::tasks
