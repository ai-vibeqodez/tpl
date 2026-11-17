#include <aw/tpl/operation_canceled_exception.hpp>

namespace aw::tpl {

OperationCanceledException::OperationCanceledException()
    : message_("Operation was canceled.") {}

const char* OperationCanceledException::what() const noexcept {
    return message_.c_str();
}

} // namespace aw::tpl
