#include <aw/tpl/timeout_exception.hpp>

namespace aw::tpl {

TimeoutException::TimeoutException()
    : message_("The operation timed out.") {}

TimeoutException::TimeoutException(const std::string& message)
    : message_(message) {}

const char* TimeoutException::what() const noexcept {
    return message_.c_str();
}

} // namespace aw::tpl
