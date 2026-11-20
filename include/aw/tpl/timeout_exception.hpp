#pragma once

#include <exception>
#include <string>

namespace aw::tpl {

/**
 * @class TimeoutException
 * @brief Thrown when an operation does not finish in time (equivalent to C# @c System.TimeoutException).
 *        Typical use: catch @c OperationCanceledException raised by a @c CancellationTokenSource with a
 *        timeout and rethrow it as @c TimeoutException.
 */
class TimeoutException : public std::exception {
public:
    /// Creates the exception with the .NET default message ("The operation timed out.").
    TimeoutException();

    /**
     * @brief Creates the exception with a custom message.
     * @param[in] message Text returned by @c what().
     */
    explicit TimeoutException(const std::string& message);

    /**
     * @brief Creates the exception with a custom message and reason.
     * @param[in] message Text returned by @c what().
     * @param[in] reason Brief description of what operation timed out.
     */
    TimeoutException(const std::string& message, const std::string& reason);

    /// Returns the exception message.
    const char* what() const noexcept override;

    /**
     * @brief Returns the reason the operation timed out.
     * @return Reason string (empty if not provided).
     */
    const std::string& exception_reason() const noexcept { return reason_; }

private:
    std::string message_;
    std::string reason_;
};

} // namespace aw::tpl
