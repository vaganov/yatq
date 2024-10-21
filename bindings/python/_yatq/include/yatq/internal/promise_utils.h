#ifndef _YATQ_INTERNAL_PROMISE_UTILS_H
#define _YATQ_INTERNAL_PROMISE_UTILS_H

#include <type_traits>

namespace yatq::internal {

template<typename result_type, typename Job, typename Promise>
std::enable_if_t<!std::is_void_v<result_type>> run_and_set_value(Job job, Promise promise) {
    promise.set_value(job());
}

template<typename result_type, typename Job, typename Promise>
std::enable_if_t<std::is_void_v<result_type>> run_and_set_value(Job job, Promise promise) {
    job();
    promise.set_value();
}

template<typename result_type, typename Future, typename Promise>
std::enable_if_t<!std::is_void_v<result_type>> get_and_set_value(Future future, Promise promise) {
    promise.set_value(future.get());
}

template<typename result_type, typename Future, typename Promise>
std::enable_if_t<std::is_void_v<result_type>> get_and_set_value(Future future, Promise promise) {
    future.get();
    promise.set_value();
}

}

#endif
