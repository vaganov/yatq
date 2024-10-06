#ifndef _YATQ_TIMER_QUEUE_H
#define _YATQ_TIMER_QUEUE_H

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#ifndef YATQ_DISABLE_FUTURES
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread/future.hpp>
#endif

#include "yatq/internal/concepts.h"
#include "yatq/internal/promise_utils.h"
#include "yatq/internal/log4cxx_proxy.h"
#include "yatq/utils/logging_utils.h"
#ifndef YATQ_DISABLE_PTHREAD
#include "yatq/utils/sched_utils.h"
#endif
#include "yatq/thread_pool.h"

namespace yatq {

using internal::ClockGeneric;
using internal::ExecutorGeneric;

template<ExecutorGeneric _Executor = ThreadPool<>, ClockGeneric _Clock = std::chrono::system_clock>
class TimerQueue {
public:
    using Clock = _Clock;
    using Executor = _Executor;
    using Executable = Executor::Executable;
    using result_type = Executable::result_type;
#ifndef YATQ_DISABLE_FUTURES
    using Future = boost::future<result_type>;  // NB: doesn't have to match 'Executor::Future'
#endif

    using uid_t = unsigned int;

    typedef struct {
        /**
         * opaque timer uid. use it to cancel the timer or to check whether it is still in queue
         */
        uid_t uid;
        /**
         * scheduled execution timepoint. added for the sake of convenience
         */
        Clock::time_point deadline;
#ifndef YATQ_DISABLE_FUTURES
        /**
         * future object. use it to obtain job result
         */
        Future result;
#endif
    } TimerHandle;

private:
#ifndef YATQ_DISABLE_FUTURES
    using Promise = boost::promise<result_type>;
#endif

    typedef struct {
        Executable job;
#ifndef YATQ_DISABLE_FUTURES
        Promise promise;
#endif
    } MapEntry;

    typedef struct {
        uid_t uid;
        Clock::time_point deadline;
    } HeapEntry;

    bool _running;
    uid_t _next_uid;
    mutable std::mutex _lock;
    std::condition_variable _cond;
    std::unordered_map<uid_t, MapEntry> _jobs;
    std::vector<HeapEntry> _heap;
    Executor* _executor;
    std::thread _thread;

public:
    /**
     * create timer queue
     * @param executor raw pointer to the job executor; cannot be \a nullptr. ownership not taken
     */
    explicit TimerQueue(Executor* executor): _running(false), _next_uid(0), _executor(executor) {}

    /**
     * start timer queue thread with default scheduling parameters
     */
    void start() {
        if (!_running) {
            _running = true;
            _thread = std::thread(&TimerQueue::demux, this);
        }
    }

#ifndef YATQ_DISABLE_PTHREAD
    /**
     * start timer queue thread with specified scheduling policy and priority
     * @param sched_policy \a SCHED_OTHER | \a SCHED_RR | \a SCHED_FIFO
     * @param priority \a yatq::utils::max_priority | \a yatq::utils::min_priority
     */
    void start(int sched_policy, utils::priority_t priority = utils::max_priority) {
        start();
        utils::set_sched_params(_thread.native_handle(), sched_policy, priority, "timer_queue");
    }

    /**
     * start timer queue thread with specified scheduling policy and priority
     * @param sched_policy \a SCHED_OTHER | \a SCHED_RR | \a SCHED_FIFO
     * @param priority explicit priority
     */
    void start(int sched_policy, int priority) {
        start();
        utils::set_sched_params(_thread.native_handle(), sched_policy, priority, "timer_queue");
    }
#endif

    /**
     * stop timer queue thread
     */
    void stop() {
        if (_running) {
            _running = false;
            _cond.notify_one();
            if (_thread.joinable()) {
                _thread.join();
            }
        }
    }

    /**
     * add timed job to the queue
     * @param deadline scheduled execution timepoint
     * @param job job to execute
     * @return timer handle to obtain result or cancel
     */
    TimerHandle enqueue(const Clock::time_point& deadline, Executable job) {
#ifndef YATQ_DISABLE_LOGGING
        static auto logger = log4cxx::Logger::getLogger("yatq.timer_queue");
#endif

#ifndef YATQ_DISABLE_FUTURES
        Promise promise;
        auto future = promise.get_future();
#endif
        uid_t uid;
        bool is_first;
        {
            std::lock_guard<std::mutex> guard(_lock);
            uid = _next_uid++;
            MapEntry map_entry {
                std::move(job)
#ifndef YATQ_DISABLE_FUTURES
                , std::move(promise)
#endif
            };
            _jobs.insert(std::make_pair(uid, std::move(map_entry)));
            _heap.push_back(HeapEntry {uid, deadline});
            std::push_heap(_heap.begin(), _heap.end(), TimerQueue::heap_cmp);
            is_first = (_heap[0].uid == uid);
        }
        if (is_first) {
            _cond.notify_one();
        }
        LOG4CXX_DEBUG(logger, "New timer uid=" + std::to_string(uid));
        return {
            uid
            , deadline
#ifndef YATQ_DISABLE_FUTURES
            , std::move(future)
#endif
        };
    }

    /**
     * cancel timed job
     * @param uid timer uid
     * @return \a true if timer was present in the queue; \a false otherwise
     */
    bool cancel(uid_t uid) {
#ifndef YATQ_DISABLE_LOGGING
        static auto logger = log4cxx::Logger::getLogger("yatq.timer_queue");
#endif

        bool was_removed;
        bool was_first;
        {
            std::lock_guard<std::mutex> guard(_lock);
            auto i = _jobs.find(uid);
            if (i != _jobs.end()) {
                LOG4CXX_DEBUG(logger, "Canceling timer uid=" + std::to_string(uid));
                _jobs.erase(i);
                was_removed = true;
                was_first = (_heap[0].uid == uid);
            }
            else {
                was_removed = false;
            }
        }
        if (was_removed && was_first) {
            _cond.notify_one();
        }
        return was_removed;
    }

    /**
     * delete all jobs from the queue
     */
    void clear() {
#ifndef YATQ_DISABLE_LOGGING
        static auto logger = log4cxx::Logger::getLogger("yatq.timer_queue");
#endif

        std::size_t total_jobs;
        std::size_t total_timers;
        {
            std::lock_guard<std::mutex> guard(_lock);
            total_jobs = _jobs.size();
            _jobs.clear();
            total_timers = _heap.size();
            _heap.clear();
        }
        if (total_jobs > 0) {
            _cond.notify_one();
        }
        auto canceled_timers = total_timers - total_jobs;
        LOG4CXX_DEBUG(logger, "Cleared " + std::to_string(total_jobs) + " timers and " + std::to_string(canceled_timers) + " canceled timers");
    }

    /**
     * delete all canceled timers from the queue
     */
    void purge() {
#ifndef YATQ_DISABLE_LOGGING
        static auto logger = log4cxx::Logger::getLogger("yatq.timer_queue");
#endif

        std::size_t canceled_timers;
        {
            std::lock_guard<std::mutex> guard(_lock);
            auto total_jobs = _jobs.size();
            auto total_timers = _heap.size();
            canceled_timers = total_timers - total_jobs;
            if (total_timers > total_jobs) {
                std::vector<HeapEntry> heap(total_jobs);
                auto i = heap.begin();
                for (auto&& heap_entry: _heap) {
                    if (_jobs.contains(heap_entry.uid)) {
                        *i++ = std::move(heap_entry);
                    }
                    else {
                        LOG4CXX_DEBUG(logger, "Timer uid=" + std::to_string(heap_entry.uid) + " has been canceled");
                    }
                }
                std::make_heap(heap.begin(), heap.end(), TimerQueue::heap_cmp);
                _heap.swap(heap);
            }
        }
        // NB: 'demux()' never waits on a canceled timer => no need to notify
        LOG4CXX_DEBUG(logger, "Purged " + std::to_string(canceled_timers) + " canceled timers");
    }

    /**
     * check whether a job is still in the queue
     * @param uid timer uid
     */
    bool in_queue(uid_t uid) const {
        std::lock_guard<std::mutex> guard(_lock);
        return _jobs.contains(uid);
    }

private:
    static bool heap_cmp(const HeapEntry& lhs, const HeapEntry& rhs) {
        return lhs.deadline > rhs.deadline;  // NB: '>'
    }

    void demux() {
#ifndef YATQ_DISABLE_LOGGING
        static auto logger = log4cxx::Logger::getLogger("yatq.timer_queue");
#endif

        SET_THREAD_TAG("timer_queue");
        LOG4CXX_INFO(logger, "Start");

        std::unique_lock<std::mutex> guard(_lock);
        while (_running) {
            bool deadline_expired = false;
            while (!_heap.empty()) {
                auto current_uid = _heap[0].uid;
                auto i = _jobs.find(current_uid);
                if (i == _jobs.end()) {
                    LOG4CXX_DEBUG(logger, "Timer uid=" + std::to_string(current_uid) + " has been canceled");
                    std::pop_heap(_heap.begin(), _heap.end(), TimerQueue::heap_cmp);
                    _heap.pop_back();
                    deadline_expired = false;
                    continue;
                }
                if (!deadline_expired) {
                    auto now = Clock::now();  // NB: system call => context switch
                    deadline_expired = (_heap[0].deadline <= now);
                }
                if (deadline_expired) {
                    LOG4CXX_DEBUG(logger, "Executing timer uid=" + std::to_string(current_uid));
                    auto node = _jobs.extract(i);
                    std::pop_heap(_heap.begin(), _heap.end(), TimerQueue::heap_cmp);
                    _heap.pop_back();
                    auto map_entry = std::move(node.mapped());

                    guard.unlock();
#ifndef YATQ_DISABLE_FUTURES
                    auto future =
#endif
                    _executor->execute(std::move(map_entry.job));
                    guard.lock();

#ifndef YATQ_DISABLE_FUTURES
                    future.then(  // future chaining -- this is why we use 'boost::future' instead of 'std::future'
                        boost::launch::sync,
                        [promise = std::move(map_entry.promise)]
                        (Executor::Future future) mutable
                        { internal::get_and_set_value<result_type>(std::move(future), std::move(promise)); }
                    );
#endif
                    deadline_expired = false;
                }
                else {
                    // NB: without this explicit cast duration type may be deduced incorrectly
                    // on Linux this leads to waiting for a random time point
                    std::chrono::time_point<Clock, typename Clock::duration> deadline = _heap[0].deadline;
                    LOG4CXX_TRACE(logger, "Wait until " + utils::time_point_to_string(deadline));
                    bool notified = _cond.wait_until(
                            guard,
                            deadline,
                            [this, current_uid] () {
                                return !_jobs.contains(current_uid) || (_heap[0].uid != current_uid) || !_running;
                            }
                    );
                    LOG4CXX_TRACE(logger, "Wake-up");
                    if (!_running) {
                        LOG4CXX_WARN(logger, "Stopping timer queue with unprocessed timers");
                        return;
                    }
                    if (!notified) {  // => timeout
                        deadline_expired = true;
                    }
                }
            }
            LOG4CXX_TRACE(logger, "Wait");
            _cond.wait(guard, [this] () { return !_heap.empty() || !_running; });
            LOG4CXX_TRACE(logger, "Wake-up");
        }

        LOG4CXX_INFO(logger, "Stop");
    }
};

}

#endif
