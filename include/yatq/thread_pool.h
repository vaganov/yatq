#ifndef _YATQ_THREAD_POOL_H
#define _YATQ_THREAD_POOL_H

#include <condition_variable>
#include <deque>
#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifndef YATQ_DISABLE_FUTURES
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>
#endif

#include "yatq/internal/concepts.h"
#ifndef YATQ_DISABLE_FUTURES
#include "yatq/internal/promise_utils.h"
#endif
#include "yatq/internal/log4cxx_proxy.h"

namespace yatq {

using internal::ExecutableGeneric;

template<ExecutableGeneric _Executable = std::function<void(void)>>
class ThreadPool {
public:
    using Executable = _Executable;
    using result_type = Executable::result_type;
#ifndef YATQ_DISABLE_FUTURES
    using Future = boost::future<result_type>;
#endif

private:
#ifndef YATQ_DISABLE_FUTURES
    using Promise = boost::promise<result_type>;
#endif

    typedef struct {
        Executable job;
#ifndef YATQ_DISABLE_FUTURES
        Promise promise;
#endif
    } QueueEntry;

    bool _running;
    std::mutex _lock;
    std::condition_variable _cond;
    std::deque<QueueEntry> _queue;
    std::vector<std::thread> _pool;

public:
    /**
     * create thread pool
     */
    ThreadPool(): _running(false) {}

    /**
     * start thread pool
     * @param num_threads number of threads
     */
    void start(std::size_t num_threads) {
        if (!_running) {
            _running = true;
            for (int i = 0; i < num_threads; ++i) {
                std::string thread_tag = std::format("pool thread #{}", i);
                auto thread = std::thread(&ThreadPool::thread_routine, this, std::move(thread_tag));
                _pool.push_back(std::move(thread));
            }
        }
    }

    /**
     * stop thread pool and join all the threads
     */
    void stop() {
        if (_running) {
            _running = false;
            _cond.notify_all();
            for (auto&& thread: _pool) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
            _pool.clear();
        }
    }

    /**
     * execute job in a thread
     * @param job job to execute
     * @return future object. use it to obtain job result
     */
#ifndef YATQ_DISABLE_FUTURES
    Future
#else
    void
#endif
    execute(Executable job) {
#ifndef YATQ_DISABLE_FUTURES
        Promise promise;
        auto future = promise.get_future();
#endif
        {
            std::lock_guard<std::mutex> guard(_lock);
            _queue.push_back({
                std::move(job)
#ifndef YATQ_DISABLE_FUTURES
                , std::move(promise)
#endif
            });
        }
        _cond.notify_one();
#ifndef YATQ_DISABLE_FUTURES
        return std::move(future);
#endif
    }

private:
    void thread_routine(std::string&& thread_tag) {
#ifndef YATQ_DISABLE_LOGGING
        static auto logger = log4cxx::Logger::getLogger("yatq.thread_pool");
#endif

        SET_THREAD_TAG(thread_tag);
        LOG4CXX_INFO(logger, "Start");

        while (_running) {
            QueueEntry queue_entry;
            {
                std::unique_lock<std::mutex> guard(_lock);
                _cond.wait(guard, [this] () { return !_queue.empty() || !_running; });
                if (!_running) {
                    break;
                }
                queue_entry = std::move(_queue.front());
                _queue.pop_front();
            }
            LOG4CXX_TRACE(logger, "Start job");
#ifndef YATQ_DISABLE_FUTURES
            internal::run_and_set_value<result_type>(std::move(queue_entry.job), std::move(queue_entry.promise));
#else
            queue_entry.job();
#endif
            LOG4CXX_TRACE(logger, "Job complete");
        }

        LOG4CXX_INFO(logger, "Stop");
    }
};

}

#endif
