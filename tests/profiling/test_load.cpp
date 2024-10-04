#include <chrono>
#include <iostream>
#include <vector>

#include <sched.h>
#include <unistd.h>

#define YATQ_DISABLE_FUTURES
#define YATQ_DISABLE_LOGGING
#include "yatq/timer_queue.h"

class InstantExecutor {
public:
    using Executable = std::function<void(void)>;

    static void execute(const Executable& job) {
        job();
    }
};

typedef yatq::TimerQueue<InstantExecutor, std::chrono::high_resolution_clock> HighResolutionTimerQueue;

void evaluate_delay(const HighResolutionTimerQueue::Clock::time_point& scheduled) {
    auto now = HighResolutionTimerQueue::Clock::now();
    auto delay = now - scheduled;
    std::clog << "delay=" << delay.count() << std::endl;
}

int main() {
    std::clog.imbue(std::locale(""));

    InstantExecutor instant_executor;
    HighResolutionTimerQueue timer_queue(&instant_executor);
    timer_queue.start(SCHED_FIFO);

    const auto N = 1'000'000;
    std::vector<HighResolutionTimerQueue::uid_t> timer_uids(N);
    auto j = timer_uids.begin();
    auto start = HighResolutionTimerQueue::Clock::now();
    auto deadline = start + std::chrono::seconds(5);

    HighResolutionTimerQueue::TimerHandle handle;
    for (auto i = 0; i < N; ++i) {
        handle = timer_queue.enqueue(deadline, [] () {});
        *j++ = handle.uid;
    }

    auto stop = HighResolutionTimerQueue::Clock::now();
    auto duration = stop - start;
    long double duration_count = duration.count();
    auto mean = duration_count / N;
    std::clog << "enqueue: " << N << " samples, mean=" << mean << std::endl;

    start = HighResolutionTimerQueue::Clock::now();
    for (auto timer_uid: timer_uids) {
        timer_queue.cancel(timer_uid);
    }
    stop = HighResolutionTimerQueue::Clock::now();
    duration = stop - start;
    duration_count = duration.count();
    mean = duration_count / N;
    std::clog << "cancel: " << N << " samples, mean=" << mean << std::endl;

    start = HighResolutionTimerQueue::Clock::now();
    timer_queue.purge();
    stop = HighResolutionTimerQueue::Clock::now();
    duration = stop - start;
    duration_count = duration.count();
    mean = duration_count / N;
    std::clog << "purge: 1 sample, " << N << " jobs, avg=" << mean << std::endl;

    timer_queue.enqueue(deadline, [deadline] () { evaluate_delay(deadline); });
    ::sleep(5);

    timer_queue.stop();

    return EXIT_SUCCESS;
}
