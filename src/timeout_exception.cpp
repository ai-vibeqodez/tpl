#include <aw/tpl/timeout_exception.hpp>

namespace aw::tpl {

TimeoutException::TimeoutException()
    : message_("The operation timed out.") {}

TimeoutException::TimeoutException(std::string message)
    : message_(std::move(message)) {}

TimeoutException::TimeoutException(std::string message, std::string reason)
    : message_(std::move(message)), reason_(std::move(reason)) {}

const char* TimeoutException::what() const noexcept {
    return message_.c_str();
}

} // namespace aw::tpl
