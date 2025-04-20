#ifndef _YATQ_INTERNAL_PROMISE_UTILS_H
#define _YATQ_INTERNAL_PROMISE_UTILS_H

#include <type_traits>

#include <boost/exception_ptr.hpp>

namespace yatq::internal {

template<typename result_type, typename Job, typename Promise>
std::enable_if_t<!std::is_void_v<result_type>> run_and_set_value(Job job, Promise promise) {
    try {
        promise.set_value(job());
    }
    catch (...) {
        promise.set_exception(boost::current_exception());
    }
}

template<typename result_type, typename Job, typename Promise>
std::enable_if_t<std::is_void_v<result_type>> run_and_set_value(Job job, Promise promise) {
    try {
        job();
        promise.set_value();
    }
    catch (...) {
        promise.set_exception(boost::current_exception());
    }
}

template<typename result_type, typename Future, typename Promise>
std::enable_if_t<!std::is_void_v<result_type>> get_and_set_value(Future future, Promise promise) {
    try {
        promise.set_value(future.get());
    }
    catch (...) {
        promise.set_exception(boost::current_exception());
    }
}

template<typename result_type, typename Future, typename Promise>
std::enable_if_t<std::is_void_v<result_type>> get_and_set_value(Future future, Promise promise) {
    try {
        future.get();
        promise.set_value();
    }
    catch (...) {
        promise.set_exception(boost::current_exception());
    }
}

}

#endif
