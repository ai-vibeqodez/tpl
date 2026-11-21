#ifndef AW_TPL_THREADING_TASKS_DETAIL_TASK_STATELESS_EXCEPTION_HPP
#define AW_TPL_THREADING_TASKS_DETAIL_TASK_STATELESS_EXCEPTION_HPP

#include <exception>
#include <string>

namespace aw::tpl::threading::tasks::detail {

/**
 * @brief Internal exception thrown when a Task has no shared state.
 */
class task_stateless_exception : public std::exception {
public:
    task_stateless_exception()
        : message_("Task has no shared state (default-constructed or moved-from).") {}

    [[nodiscard]] const char* what() const noexcept override {
        return message_.c_str();
    }

private:
    std::string message_;
};

} // namespace aw::tpl::threading::tasks::detail

#endif  // AW_TPL_THREADING_TASKS_DETAIL_TASK_STATELESS_EXCEPTION_HPP
