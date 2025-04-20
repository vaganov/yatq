#include <chrono>
#include <cstdlib>
#include <exception>
#include <format>
#include <string>

#include <sched.h>

#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>

#include "yatq/thread_pool.h"
#include "yatq/timer_queue.h"
#include "yatq/internal/log4cxx_proxy.h"

static auto logger = log4cxx::Logger::getLogger("examples.yatq");

struct C {
    std::string s;
};

int main() {
    log4cxx::xml::DOMConfigurator::configure("log4cxx.xml");
    SET_THREAD_TAG("main");

    yatq::ThreadPool<std::function<C(void)>> thread_pool;
    yatq::TimerQueue timer_queue(&thread_pool);

    thread_pool.start(8);
    timer_queue.start(SCHED_FIFO);

    auto now = std::chrono::system_clock::now();
    auto deadline = now + std::chrono::milliseconds(100);
    auto handle = timer_queue.enqueue(deadline, [] () { return C {"test"}; });
    auto another_handle = timer_queue.enqueue(deadline, [] () { return C {"won't make it"}; });
    auto exc_handle = timer_queue.enqueue(deadline, [] () -> C { throw std::runtime_error("test"); });
    timer_queue.cancel(another_handle.uid);
    auto return_value = handle.result.get();
    LOG4CXX_INFO(logger, std::format("return_value={}", return_value.s));
    try {
        exc_handle.result.get();
    }
    catch (const std::exception& exc) {
        LOG4CXX_ERROR(logger, std::format("exception={}", std::string(exc.what())));
    }

    timer_queue.stop();
    thread_pool.stop();

    auto root_logger = log4cxx::Logger::getRootLogger();
    auto async_appender = root_logger->getAppender("AsyncAppender");
    if (async_appender) {
        async_appender->close();
    }
    return EXIT_SUCCESS;
}
