#include <aw/tpl/threading/tasks/task_already_moved_exception.hpp>

namespace aw::tpl::threading::tasks {

TaskAlreadyMovedException::TaskAlreadyMovedException()
    : message_("Task move-only value has already been moved out via co_await or result()."),
      reason_("Result was already extracted in a prior co_await or result() call.") {}

const char* TaskAlreadyMovedException::what() const noexcept {
    return message_.c_str();
}

} // namespace aw::tpl::threading::tasks
