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

    /// Returns the exception message.
    const char* what() const noexcept override;

private:
    std::string message_;
};

} // namespace aw::tpl
