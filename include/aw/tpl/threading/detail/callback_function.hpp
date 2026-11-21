#pragma once

#if __has_include(<version>)
#include <version>
#endif

#include <functional>

namespace aw::tpl::threading::detail {

/// Type-erased callback: std::move_only_function when available, std::function as fallback.
template <typename Signature>
#if defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L
using CallbackFunction = std::move_only_function<Signature>;
#else
using CallbackFunction = std::function<Signature>;
#endif

} // namespace aw::tpl::threading::detail
