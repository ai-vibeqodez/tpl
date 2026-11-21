#include <aw/tpl/operation_canceled_exception.hpp>

namespace aw::tpl {

OperationCanceledException::OperationCanceledException()
    : message_("Operation was canceled.") {}

OperationCanceledException::OperationCanceledException(std::string reason)
    : message_("Operation was canceled."), reason_(std::move(reason)) {}

const char* OperationCanceledException::what() const noexcept {
    return message_.c_str();
}

} // namespace aw::tpl
