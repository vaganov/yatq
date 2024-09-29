#ifndef _YATQ_INTERNAL_CONCEPTS_H
#define _YATQ_INTERNAL_CONCEPTS_H

#include <chrono>
#include <concepts>

namespace yatq::internal {

// template<typename Clock>
// concept ClockGeneric = std::chrono::is_clock<Clock>;

// 'std::chrono::is_clock' seems to be not yet implemented for all architectures
// ok, we only these from 'Clock'
template<typename Clock>
concept ClockGeneric = requires (Clock::time_point t1, Clock::time_point t2) {
    typename Clock::duration;
    typename Clock::time_point;
    { Clock::now() } -> std::convertible_to<typename Clock::time_point>;
    { t1 > t2 } -> std::convertible_to<bool>;
    { t1 <= t2 } -> std::convertible_to<bool>;
#ifndef YATQ_DISABLE_LOGGING
    t1.time_since_epoch().count();
#endif
};

template<typename Executable>
concept ExecutableGeneric = requires {
    typename Executable::result_type;
    std::invocable<Executable>;
    std::movable<Executable>;
};

#ifndef YATQ_DISABLE_FUTURES
template<typename Future, typename result_type>
concept ChainableFutureGeneric = requires(Future future, void (*then) (Future)) {
    std::movable<Future>;
    { future.get() } -> std::convertible_to<result_type>;
    future.then(then);
};
#endif

template<typename Executor>
concept ExecutorGeneric =
#ifndef YATQ_DISABLE_FUTURES
        ChainableFutureGeneric<typename Executor::Future, typename Executor::Executable::result_type> &&
#endif
        requires(Executor executor, Executor::Executable job) {
    typename Executor::Executable;
    typename Executor::Executable::result_type;
    std::movable<typename Executor::Executable>;
#ifndef YATQ_DISABLE_FUTURES
    typename Executor::Future;
    { executor.execute(job) } -> std::convertible_to<typename Executor::Future>;
#else
    executor.execute(job);
#endif
};

}

#endif
