#ifndef _YATQ_BINDINGS_PYTHON_YATQ_TRAITS_H
#define _YATQ_BINDINGS_PYTHON_YATQ_TRAITS_H

#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/pybind11.h>

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>

namespace pybind11::detail {

// only needed for 'result_type = void'
template <>
struct is_move_constructible<boost::future<void>>: std::true_type {};

}

#endif
