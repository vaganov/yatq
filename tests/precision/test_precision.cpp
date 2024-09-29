#include <algorithm>
#include <chrono>
#include <functional>
#include <fstream>
#include <cstdlib>
#include <vector>

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

void store_delay(
        const HighResolutionTimerQueue::Clock::time_point& scheduled,
        std::vector<HighResolutionTimerQueue::Clock::duration::rep>& delays
) {
    auto now = HighResolutionTimerQueue::Clock::now();
    auto delay = now.time_since_epoch().count() - scheduled.time_since_epoch().count();
    delays.push_back(delay);
}

int main() {
    InstantExecutor instant_executor;
    HighResolutionTimerQueue timer_queue(&instant_executor);
    timer_queue.start(SCHED_FIFO);

    std::vector<HighResolutionTimerQueue::Clock::duration::rep> delays;

    auto deadline = HighResolutionTimerQueue::Clock::now();
    for (int i = 0; i < 1'000; ++i) {
        deadline += std::chrono::milliseconds(10);
        timer_queue.enqueue(deadline, [deadline, &delays] () { store_delay(deadline, delays); });
    }

    ::sleep(10);

    timer_queue.stop();

    std::ofstream output("tq_delays.dat");
    std::ostream_iterator<HighResolutionTimerQueue::Clock::duration::rep> output_iterator(output, ",");
    std::ranges::copy(delays, output_iterator);

    return EXIT_SUCCESS;
}
