# aw::tpl Exmplaes


| Example | Covers |
| --- | --- |
| [`01_tasks_basics.cpp`](examples/01_tasks_basics.cpp) | `Task<T>`/`Task<void>`, `co_await`, `wait()`/`result()`, exceptions, move-only results |
| [`02_task_run.cpp`](examples/02_task_run.cpp) | `Task<>::run` with lambdas, functors and move-only callables |
| [`03_when_all.cpp`](examples/03_when_all.cpp) | variadic and vector `when_all`, `Task<void>` children, failure handling |
| [`04_when_any.cpp`](examples/04_when_any.cpp) | `Task<Task<T>>` winners, `when_any` + cancel the rest, mixed types, failed winners, empty input |
| [`05_cancellation.cpp`](examples/05_cancellation.cpp) | `CancellationTokenSource`, tokens, callbacks, `cancel_after`, `async_sleep` |
| [`06_thread_pool.cpp`](examples/06_thread_pool.cpp) | `ThreadPool::global`, `set_default_threads`, private pools, blocking guard |
| [`07_timeout.cpp`](examples/07_timeout.cpp) | `CancellationTokenSource(timeout)`, `OperationCanceledException` → `TimeoutException` |
| [`08_co_spawn.cpp`](examples/08_co_spawn.cpp) | `co_spawn` fire-and-forget, error handler, `TaskGroup` draining |
