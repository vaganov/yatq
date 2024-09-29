#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/pybind11.h>
#include <pybind11/chrono.h>
#include <pybind11/functional.h>

#ifndef YATQ_DISABLE_FUTURES
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>
#endif

#include "yatq/thread_pool.h"
#include "yatq/timer_queue.h"
#include "yatq/version.h"

#include "traits.h"

namespace py = pybind11;

using result_type = py::object;
using Future = boost::future<result_type>;
using Executable = std::function<result_type(void)>;
using ThreadPool = yatq::ThreadPool<Executable>;
using TimerQueue = yatq::TimerQueue<ThreadPool>;

PYBIND11_MODULE(_yatq, m) {
#ifndef YATQ_DISABLE_FUTURES
    auto boost_submodule = m.def_submodule("boost");
    py::class_<Future>(boost_submodule, "future")
        .def("get", &Future::get, py::call_guard<py::gil_scoped_release>())
        .def("wait", &Future::wait, py::call_guard<py::gil_scoped_release>())
        .def("is_ready", py::overload_cast<>(&Future::is_ready, py::const_));
#endif

#ifndef YATQ_DISABLE_PTHREAD
    auto utils_submodule = m.def_submodule("utils");
    py::enum_<yatq::utils::priority_t>(utils_submodule, "priority_t")
        .value("min_priority", yatq::utils::min_priority)
        .value("max_priority", yatq::utils::max_priority)
        .export_values();
#endif

    py::class_<ThreadPool>(m, "ThreadPool")
        .def(py::init<>())
        .def("start", &ThreadPool::start, py::arg("num_threads"))
        .def("stop", &ThreadPool::stop)
        .def("execute", &ThreadPool::execute, py::arg("job"));

    py::class_<TimerQueue::TimerHandle>(m, "TimerHandle")
        .def_readwrite("uid", &TimerQueue::TimerHandle::uid)
        .def_readwrite("deadline", &TimerQueue::TimerHandle::deadline)
#ifndef YATQ_DISABLE_FUTURES
        .def_readonly("result", &TimerQueue::TimerHandle::result)
#endif
        ;

    py::class_<TimerQueue>(m, "TimerQueue")
        .def(py::init<ThreadPool*>(), py::arg("executor"))
        .def("start", py::overload_cast<>(&TimerQueue::start))
#ifndef YATQ_DISABLE_PTHREAD
        .def("start", py::overload_cast<int, yatq::utils::priority_t>(&TimerQueue::start), py::arg("sched_policy"), py::arg("priority") = yatq::utils::max_priority)
        .def("start", py::overload_cast<int, int>(&TimerQueue::start), py::arg("sched_policy"), py::arg("priority"))
#endif
        .def("stop", &TimerQueue::stop)
        .def("enqueue", &TimerQueue::enqueue, py::arg("deadline"), py::arg("job"))
        .def("cancel", &TimerQueue::cancel, py::arg("uid"))
        .def("clear", &TimerQueue::clear)
        .def("purge", &TimerQueue::purge)
        .def("in_queue", &TimerQueue::in_queue, py::arg("uid"));

    m.attr("__version__") = YATQ_VERSION;
}
