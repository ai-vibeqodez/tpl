#include <aw/tpl/timeout_exception.hpp>

namespace aw::tpl {

TimeoutException::TimeoutException()
    : message_("The operation timed out."), reason_("") {}

TimeoutException::TimeoutException(const std::string& message)
    : message_(message), reason_("") {}

TimeoutException::TimeoutException(const std::string& message, const std::string& reason)
    : message_(message), reason_(reason) {}

const char* TimeoutException::what() const noexcept {
    return message_.c_str();
}

} // namespace aw::tpl
