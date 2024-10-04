#include <bdlmt_eventscheduler.h>
#include <bslmt_threadattributes.h>
#include <bsl_chrono.h>
#include <bsl_iostream.h>
#include <bsl_locale.h>
#include <bsl_vector.h>

#include <unistd.h>

using namespace BloombergLP;

void evaluate_delay(const bsl::chrono::high_resolution_clock::time_point& scheduled) {
    auto now = bsl::chrono::high_resolution_clock::now();
    auto delay = now - scheduled;
    bsl::clog << "delay=" << delay.count() << bsl::endl;
}

int main() {
    bsl::clog.imbue(bsl::locale(""));

    bdlmt::EventScheduler event_scheduler;
    bslmt::ThreadAttributes thread_attrs;
    thread_attrs.setSchedulingPolicy(bslmt::ThreadAttributes::SchedulingPolicy::e_SCHED_FIFO);
    event_scheduler.start(thread_attrs);

    const auto N = 1'000'000;
    bsl::vector<bdlmt::EventScheduler::EventHandle> timer_handles(N);
    auto start = bsl::chrono::high_resolution_clock::now();
    auto deadline = start + bsl::chrono::seconds(5);
    for (auto& timer_handle: timer_handles) {
        event_scheduler.scheduleEvent(&timer_handle, deadline, [] () {});
    }
    auto stop = bsl::chrono::high_resolution_clock::now();
    auto duration = stop - start;
    long double duration_count = duration.count();
    auto mean = duration_count / N;
    bsl::clog << "scheduleEvent: " << N << " samples, mean=" << mean << bsl::endl;

    start = bsl::chrono::high_resolution_clock::now();
    for (auto& timer_handle: timer_handles) {
        event_scheduler.cancelEvent(&timer_handle);
    }
    stop = bsl::chrono::high_resolution_clock::now();
    duration = stop - start;
    duration_count = duration.count();
    mean = duration_count / N;
    bsl::clog << "cancelEvent: " << N << " samples, mean=" << mean << bsl::endl;

    bdlmt::EventScheduler::EventHandle timer_handle;
    event_scheduler.scheduleEvent(&timer_handle, deadline, [deadline] () { evaluate_delay(deadline); });

    ::sleep(5);

    event_scheduler.stop();

    return EXIT_SUCCESS;
}
