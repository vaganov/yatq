# Yet Another Timer Queue
Assume one needs to execute a job (i.e. to call a function) at a specified moment. In modern _C++_ this may be achieved
quite easily with a snippet like

    std::async([deadline, job] () { std::this_thread::sleep_until(deadline); return job(); });

This approach, however, does not scale really well for a large number of timers as one may eventually need way too many
threads (thread pooling is not a remedy because sufficiently many timers would effectively block all the threads in the
pool).

The good news is we do not actually need to wait for all the timers, just for the first one in the queue -- which may of
course be done within a single thread.

**yatq** provides a single-threaded timer demultiplexer able to keep track of arbitrary number of timed jobs. Unlike
[boost::asio::detail::timer_queue](https://www.boost.org/doc/libs/release/boost/asio/detail/timer_queue.hpp) it also
comes with an executor to actually run jobs.

**yatq** comes as a header-only _C++20_ library along with _python>=3.7_ embedding backed by
[pybind11](https://pybind11.readthedocs.io/en/stable/index.html).

## Table of contents
- [Usage](#usage)
  - [Basic usage (C++)](#basic-usage-c)
  - [Basic usage (python)](#basic-usage-python)
  - [API notes (C++)](#api-notes-c)
    - [Executor](#executor)
    - [Deadlines](#deadlines)
    - [Job parameters](#job-parameters)
    - [Thread safety](#thread-safety)
    - [Algorithmic complexity](#algorithmic-complexity)
  - [Advanced usage (C++)](#advanced-usage-c)
    - [Canceling timers](#canceling-timers)
    - [Template parameters](#template-parameters)
    - [Job return values](#job-return-values)
    - [Scheduling tweaks](#scheduling-tweaks)
  - [Advanced usage (python)](#advanced-usage-python)
    - [Canceling timers](#canceling-timers-1)
    - [Awaiting return value](#awaiting-return-value)
    - [Scheduling tweaks](#scheduling-tweaks-1)
  - [Timer precision](#timer-precision)
- [Prerequisites and dependencies (C++)](#prerequisites-and-dependencies-c)
  - [C++20](#c20)
  - [boost](#boost)
  - [log4cxx](#log4cxx)
  - [pthread](#pthread)
  - [cmake](#cmake)
  - [BDE](#bde)
- [Prerequisites and dependencies (python)](#prerequisites-and-dependencies-python)
  - [python 3.7+](#python-37)
  - [cmake](#cmake-1)
  - [pybind11](#pybind11)
  - [pytest](#pytest)
  - [pytest-asyncio](#pytest-asyncio)
- [Build and install (C++)](#build-and-install-c)
  - [cmake](#cmake-2)
  - [shell](#shell)
  - [running tests](#running-tests)
- [Build and install (python)](#build-and-install-python)
  - [running tests](#running-tests-1)
- [Comparison with analogs](#comparison-with-analogs)
  - [bdlmt::EventScheduler](#bdlmt---eventscheduler)

## Usage
### Basic usage (C++)

    #include <chrono>
    #include <cstdlib>
    #include <iostream>

    #include <unistd.h>

    #include <yatq/thread_pool.h>
    #include <yatq/timer_queue.h>

    int main() {
        yatq::ThreadPool thread_pool;
        yatq::TimerQueue timer_queue(&thread_pool);
    
        thread_pool.start(8);
        timer_queue.start();
    
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
        timer_queue.enqueue(deadline, [] () { std::cout << "Hello" << std::endl; });

        ::sleep(1);
    
        timer_queue.stop();
        thread_pool.stop();

        return EXIT_SUCCESS;
    }

### Basic usage (python)

    from datetime import datetime, timedelta
    import time

    from pytq import ThreadPool, TimerQueue


    def main():
        thread_pool = ThreadPool()
        timer_queue = TimerQueue(executor=thread_pool)
        
        thread_pool.start(num_threads=1)  # python jobs are going to acquire GIL anyway
        timer_queue.start()
        
        deadline = datetime.now() + timedelta(milliseconds=100)
        timer_queue.enqueue(deadline=deadline, job=lambda: print('Hello'))
        
        time.sleep(1)
        
        timer_queue.stop()
        thread_pool.stop()


    if __name__ == '__main__':
        main()

### API notes (C++)
#### Executor
For the sake of flexibility executor is a separate object. Note that `TimerQueue` takes a raw pointer to an executor and
does not take ownership:

    explicit TimerQueue(Executor* executor);

#### Deadlines
`TimerQueue` sorts timed jobs by their deadlines. To avoid jitter brought by `Clock::now()` (potentially a system call),
duration arguments are not accepted by `enqueue()`. To use duration you may add it to `TimerQueue::Clock::now()` (see
basic usage example).

#### Job parameters
Neither `ThreadPool::execute()` nor `TimerQueue::enqueue()` take jobs with variadic arguments. To pass arbitrary
arguments to a timed job you would need to bind them (say, with a lambda or
[std::bind()](https://en.cppreference.com/w/cpp/utility/functional/bind)). Make sure that your arguments are movable
between threads.

#### Thread safety
Methods `TimerQueue::enqueue()`, `TimerQueue::cancel()`, `TimerQueue::clear()`, `TimerQueue::purge()`,
`TimerQueue::in_queue()` and `ThreadPool::execute()` are thread-safe.

#### Algorithmic complexity
`TimerQueue` stores jobs in an unordered map and timers in a heap. Whilst jobs are removed right away upon canceling,
canceled timers are only deleted in these cases to avoid heap searching:
- a canceled timer becomes first in the queue and is deleted by the timer queue thread
- `clear()` is called and all jobs and timers are deleted
- `purge()` is called and all canceled timers are deleted

Assume there are `N` non-canceled and `M` canceled timers (and therefore `N` jobs). Then `TimerQueue` methods have the
following algorithmic complexity:

| method     | time (average) | time (worst case) |memory| postcondition |
|------------|----------------|-------------------|------|---------------|
| `enqueue`  | `O(ln(N + M))` | `O(N + ln M)`     |`O(1)`| `N >= 1`      |
| `cancel`   | `O(1)`         | `O(N)`            |`O(1)`| `M >= 1`      |
| `clear`    | `O(N + M)`     | `O(N + M)`        |`O(1)`| `N = M = 0`   |
| `purge`    | `O(N + M)`     | `O(N (N + M))`    |`O(N)`| `M = 0`       |
| `in_queue` | `O(1)`         | `O(N)`            |`O(1)`|               |

Consider calling `purge()` periodically if the application cancels way more timed jobs in the far future than reach the
execution. Canceling many jobs in the near future is not the case though as such timers will be purged by the timer
queue thread.

### Advanced usage (C++)
#### Canceling timers
A timed job can be canceled through a hangle returned by `enqueue()`:

    auto handle = timer_queue.enqueue(deadline, job);
    bool canceled = timer_queue.cancel(handle.uid);

(A job already executed or passed to executor cannot be canceled, in which case `cancel()` returns `false`)

#### Template parameters
`TimerQueue` is a template class parametrized with `Clock` and `Executor` types. Since deadlines are going to be passed
to [std::condition_variable::wait_until()](https://en.cppreference.com/w/cpp/thread/condition_variable/wait_until),
`Clock` shall be one of `std::chrono` clock types (`std::chrono::system_clock` by default). `Executor` may be any type
matching `ExecutorGeneric` concept (see [<yatq/internal/concepts.h>](include/yatq/internal/concepts.h)):

    template<typename Executor>
    concept ExecutorGeneric =
            ChainableFutureGeneric<typename Executor::Future, typename Executor::Executable::result_type> &&
            requires(Executor executor, Executor::Executable job) {
        typename Executor::Executable;
        typename Executor::Executable::result_type;
        std::movable<typename Executor::Executable>;
        typename Executor::Future;
        { executor.execute(job) } -> std::convertible_to<typename Executor::Future>;
    };

**yatq** comes with a default `Executor` implementation: `ThreadPool` -- which is in turn a template class parametrized
with `Executable`, a type matching `ExecutableGeneric` concept:

    template<typename Executable>
    concept ExecutableGeneric = requires {
        typename Executable::result_type;
        std::invocable<Executable>;
        std::movable<Executable>;
    };

(`std::function<void(void)>` being an obvious default). This said, `TimerQueue` may be instantiated with:
- another `std::chrono` clock
- `ThreadPool` with another executable
- another executor

For an example of different instantiation see [tests/precision/test_precision.cpp](tests/precision/test_precision.cpp).

#### Job return values
Both `ThreadPool` and `TimerQueue` provide job return values through futures:

    #include <chrono>
    #include <cstdlib>
    #include <functional>
    #include <iostream>

    #include <yatq/thread_pool.h>
    #include <yatq/timer_queue.h>

    int main() {
        yatq::ThreadPool<std::function<int(void)>> thread_pool;
        yatq::TimerQueue timer_queue(&thread_pool);
    
        thread_pool.start(8);
        timer_queue.start();
    
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
        auto handle = timer_queue.enqueue(deadline, [] () { return 1; });

        auto return_value = handle.result.get();  // NB: blocking call
        std::cout << "return_value=" << return_value << std::endl;
    
        timer_queue.stop();
        thread_pool.stop();

        return EXIT_SUCCESS;
    }

#### Scheduling tweaks
(Assuming OS user has sufficient privileges) `TimerQueue` may be started with specified POSIX scheduling policy and
thread priority (`yatq::utils::max_priority` by default). For time sensitive applications it is highly recommended to
use `SCHED_FIFO` policy:

    #include <sched.h>

    ...

    timer_queue.start(SCHED_FIFO);

### Advanced usage (python)
Since uninstantiated _C++_ templates do not generate object code, they cannot be embedded in _python_; only `ThreadPool`
instantiated with `Executable = std::function<pybind11::object(void)>` and `TimerQueue` instantiated with that
`ThreadPool` and `Clock = std::chrono::system_clock` are embedded in **pytq**. Hence, only `pytq.ThreadPool` object may
be passed as an executor and only `datetime` object may be passed as `deadline` to `pytq.TimerQueue`. However, all the
features are available.

#### Canceling timers

    handle = timer_queue.enqueue(deadline=deadline, job=job)
    canceled = timer_queue.cancel(uid=handle.uid)

#### Awaiting return value
A function returning any _python_ entity (`None`, a scalar or an object) may be enqueued. Arguments, however, should be
bound (say, with a lambda or [functools.partial](https://docs.python.org/3/library/functools.html#functools.partial))
prior to passing to `pytq.TimerQueue`.

    handle = timer_queue.enqueue(deadline=deadline, job=job)
    return_value = handle.result.get()  # NB: blocking call

It is worth noting that although `handle.result.get()` is a blocking call it will not block _python_ jobs called from
other threads as it releases GIL upon entering _C++_ code (and so does `handle.result.wait()`). Also, thanks to
[boost::future::is_ready()](https://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html#thread.synchronization.futures.reference.unique_future.is_ready),
a non-blocking async wrapper is available:

    import asyncio
    from datetime import datetime, timedelta

    from pytq import ThreadPool, TimerQueue, AwaitableFuture


    async def main():
        thread_pool = ThreadPool()
        timer_queue = TimerQueue(executor=thread_pool)
        
        thread_pool.start(num_threads=1)
        timer_queue.start()
        
        deadline = datetime.now() + timedelta(milliseconds=100)
        handle = timer_queue.enqueue(deadline=deadline, job=lambda: print('Hello'))
        
        future = AwaitableFuture(handle.result)
        return_value = await future  # yields until result is ready
        # return_value = await future.get()  # same, but eligible for 'asyncio.create_task()' etc.
        
        timer_queue.stop()
        thread_pool.stop()


    if __name__ == '__main__':
        asyncio.run(main())

Please be aware that `AwaitableFuture.__await__()` and `AwaitableFuture.get()` are essentially polls, that is in a
non-concurrent application blocking on `handle.result.get()` would be more efficient CPU-wise.

    def __await__(self):
        while not self._future.is_ready():
            yield
        return self._future.get()

    async def get(self):
        while not self._future.is_ready():
            await asyncio.sleep(0)
        return self._future.get()

#### Scheduling tweaks

    import os

    ...

    timer_queue.start(sched_policy=os.SCHED_FIFO)

### Timer precision
How precise is timer? In other words, what are expected delays between specified deadline and actual execution?

Apparently delays depend on the machine architecture and especially on the OS scheduler. To see delay distribution on a
particular machine, run **test_precision** test: it runs for about 10 sec and saves delay samples as **tq_delays.dat**
in the working directory. The test instantiates `TimerQueue` with a synchronous executor and
`std::chrono::high_resolution_clock` and starts the timer queue with `SCHED_FIFO` scheduling policy and maximum
priority; the logging is compiled out.

Delay samples may be analyzed with any statistical tool. Please find a
[jupyter notebook](tests/precision/delay_histogram.ipynb) to draw a histogram:

    []: %matplotlib inline

    []: import matplotlib.pyplot as plt

    []: import numpy as np

    []: from scipy.stats.mstats import winsorize

    []: tq_delays = np.fromfile('tq_delays.dat', sep=',')

    []: tq_delays.min(), tq_delays.max()

    []: tq_delays.mean(), tq_delays.std(ddof=1)  # Bessel’s correction

    []: _ = plt.hist(winsorize(tq_delays, limits=(0, 0.01)), bins=100)  # you may want to winsorize the tail percentile

Timer delays are mostly brought by underlying
[std::condition_variable::wait_until()](https://en.cppreference.com/w/cpp/thread/condition_variable/wait_until). To see
its delay distribution on a particular machine, run **test_wait_until** test: it runs for about 10 sec and saves delay
samples as **cv_delays.dat** in the working directory. The test does not involve **yatq** except for calling

    yatq::utils::set_sched_params(pthread_self(), SCHED_FIFO, yatq::utils::max_priority);

`std::chrono::high_resolution_clock` is being used as clock. Delay samples may be visualized with the same
[jupyter notebook](tests/precision/delay_histogram.ipynb).

## Prerequisites and dependencies (C++)
Please refer to your OS package manager documentation on how to install the dependencies.

### C++20
Being a template library, **yatq** relies heavily on [concepts](https://en.cppreference.com/w/cpp/language/constraints)
which have only been introduced in _C++20_. There is no way to run this library on earlier standards without editing.

### [boost](https://www.boost.org/doc/libs/release/more/getting_started/index.html)
In order to chain futures returned by executor with futures returned by `TimerQueue` we need a
[continuation mechanism](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3634.pdf)
which has not yet been adopted by C++ standard but has been
[implemented](https://www.boost.org/doc/libs/release/doc/html/thread/synchronization.html#thread.synchronization.futures.then)
in **boost**. If using **boost** is undesirable, one may define `YATQ_DISABLE_FUTURES` macro (in which case the
possibility to obtain job results will be lost):

    #define YATQ_DISABLE_FUTURES
    #include <yatq/timer_queue.h>

### [log4cxx](https://logging.apache.org/log4cxx/latest_stable/download.html)
**log4cxx** is being used as a logging engine. If a multithreaded executor and an asynchronous appender (which is highly
recommended for time sensitive applications) are used, passing some sort of thread identifier to logging might be very
useful. For that purpose **yatq** threads use `threadTag` key in
[log4cxx::MDC](https://logging.apache.org/log4cxx/latest_stable/classlog4cxx_1_1MDC.html). See included
[log4cxx.xml](log4cxx.xml) config for usage example. If using **log4cxx** is undesirable, one may define
`YATQ_DISABLE_LOGGING` macro (in which case no logs will be produced):

    #define YATQ_DISABLE_LOGGING
    #include <yatq/timer_queue.h>

### pthread
**pthread** is being used to set timer queue thread scheduling parameters. If using **pthread** is undesirable, one may
define `YATQ_DISABLE_PTHREAD` macro (in which case `TimerQueue` may only be started with default scheduling parameters):

    #define YATQ_DISABLE_PTHREAD
    #include <yatq/timer_queue.h>

### [cmake](https://cmake.org/download)
**cmake** is being used for building tests and installation (the latter may be done by simply copying the headers).

### [BDE](http://bloomberg.github.io/bde/library_information/build.html)
**BDE** is only used in comparison tests. It has nothing to do with **yatq**.

## Prerequisites and dependencies (python)
Since _python_ wrapper is essentially a dynamic library, all _C++_ dependencies apply as well. _python_ dependencies may
be installed all at once with

    (venv) $ pip install -r bindings/python/requirements.txt

### python 3.7+
_python_ wrapper has been tested on versions _python3.7_ -- _python3.12_ as older versions are out of support.

### [cmake](https://cmake.org/download)
**cmake** is being used as a setup helper.

### pybind11
**pybind11** is being used to embed _C++_ classes in _python_.

### pytest
**pytest** is being used as a testing framework.

### pytest-asyncio
**pytest-asyncio** is a **pytest** plugin for async tests.

## Build and install (C++)
Since **yatq** is a template library, there is no build stage as such (apart from the tests).

### cmake

    $ git clone https://github.com/vaganov/yatq
    $ cd yatq
    $ cmake .
    $ sudo cmake --install .

### shell

    $ git clone https://github.com/vaganov/yatq
    $ cd yatq
    $ sudo cp -r include/yatq /usr/local/include

### running tests

    $ cd cmake-build  # YMMV
    $ make
    $ ctest

## Build and install (python)
(Assuming target _python_ environment has been activated, if any)

    (venv) $ git clone https://github.com/vaganov/yatq
    (venv) $ cd yatq
    (venv) $ pip install -r bindings/python/requirements.txt
    (venv) $ pip install wheel setuptools pip --upgrade
    (venv) $ pip install bindings/python

[(why step 4)](https://stackoverflow.com/questions/61235727/no-module-named-pybind11-after-installing-pybind11)

Note that _python_ package is named **pytq**. Technically this is [squatting](https://pypi.org/project/pytq). If you
believe this may be an issue for your _python_ environment, please upvote [issue #1](https://github.com/vaganov/yatq/issues/1).

### running tests

    (venv) $ pytest tests/python

## Comparison with analogs
Despite "yet another" in the name, not many analogs may be found out there. However,
[BDE](http://bloomberg.github.io/bde/index.html)'s
[bdlmt::EventScheduler](https://bloomberg.github.io/bde-resources/doxygen/bde_api_prod/classbdlmt_1_1EventScheduler.html)
provides a notable example.

### [bdlmt::EventScheduler](https://bloomberg.github.io/bde-resources/doxygen/bde_api_prod/classbdlmt_1_1EventScheduler.html)
The main difference between **yatq** and **BDE**'s component is that `bdlmt::EventScheduler` provides neither return
values nor execution notifications. Apart from this, interfaces (expectedly) pretty much match each other.
`bdlmt::EventScheduler` is also capable of scheduling recurring events.

Runtime-wise, when built in release mode with `YATQ_DISABLE_FUTURES` macro, `yatq::TimerQueue::enqueue()` is typically
couple times faster than `bdlmt::EventScheduler::scheduleEvent()` and `yatq::TimerQueue::cancel()` is insignificantly
faster than `bdlmt::EventScheduler::cancelEvent()`. Run **test_load** and **test_bde** tests for more details on a
particular machine (please note that **test_bde** requires **BDE** to be installed).
