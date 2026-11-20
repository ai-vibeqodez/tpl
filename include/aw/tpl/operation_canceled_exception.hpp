#pragma once

#include <exception>
#include <string>

namespace aw::tpl {

/**
 * @class OperationCanceledException
 * @brief Exception thrown when an asynchronous operation is canceled via a @c CancellationToken
 *        (equivalent to C# @c System.OperationCanceledException).
 */
class OperationCanceledException : public std::exception {
public:
    /// Creates the exception with the .NET default message ("Operation was canceled.").
    OperationCanceledException();

    /**
     * @brief Creates the exception with a custom reason.
     * @param[in] reason Brief description of why the operation was canceled.
     */
    explicit OperationCanceledException(const std::string& reason);

    /// Returns the exception message.
    const char* what() const noexcept override;

    /**
     * @brief Returns the reason the operation was canceled.
     * @return Reason string (empty if created with the default message).
     */
    const std::string& exception_reason() const noexcept { return reason_; }

private:
    std::string message_;
    std::string reason_;
};

} // namespace aw::tpl
