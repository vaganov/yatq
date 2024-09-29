#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <vector>

#include <pthread.h>

#include <sched.h>

#define YATQ_DISABLE_LOGGING
#include "yatq/utils/sched_utils.h"

typedef std::chrono::high_resolution_clock Clock;

int main() {
    yatq::utils::set_sched_params(pthread_self(), SCHED_FIFO, yatq::utils::max_priority);

    std::mutex lock;
    std::unique_lock<std::mutex> guard(lock);
    std::condition_variable cond;

    std::vector<Clock::duration::rep> delays;

    for (int i = 0; i < 1'000; ++i) {
        Clock::time_point until = Clock::now() + std::chrono::milliseconds(10);
        cond.wait_until(guard, until);
        Clock::time_point now = Clock::now();
        auto delay = now.time_since_epoch().count() - until.time_since_epoch().count();
        delays.push_back(delay);
    }

    std::ofstream output("cv_delays.dat");
    std::ostream_iterator<Clock::duration::rep> output_iterator(output, ",");
    std::ranges::copy(delays, output_iterator);

    return EXIT_SUCCESS;
}
