#ifndef _YATQ_UTILS_LOGGING_UTILS_H
#define _YATQ_UTILS_LOGGING_UTILS_H

#include <chrono>
#include <iomanip>
#include <locale>
#include <sstream>

#include <sched.h>

namespace yatq::utils {

template<typename time_point>
std::string time_point_to_string(const time_point& t) {
    return std::to_string(t.time_since_epoch().count());
}

template<>
inline std::string time_point_to_string(const std::chrono::system_clock::time_point& time_point) {
    std::stringstream stream;

    std::time_t timestamp = std::chrono::system_clock::to_time_t(time_point);
    std::tm* tm_info_p = std::localtime(&timestamp);  // static initialization, no need for memory management
    stream << std::put_time(tm_info_p, "%F %T");

    // locale-aware decimal point
    static const char decimal_point = std::use_facet<std::numpunct<char>>(std::locale("")).decimal_point();

    auto time_point_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(time_point);
    auto milliseconds = time_point_ms.time_since_epoch().count() % 1000;
    stream << decimal_point << std::setfill('0') << std::setw(3) << milliseconds;

    return stream.str();
}

inline std::string sched_policy_to_string(int sched_policy) {
    switch (sched_policy) {
        case SCHED_OTHER:
            return "SCHED_OTHER";
        case SCHED_FIFO:
            return "SCHED_FIFO";
        case SCHED_RR:
            return "SCHED_RR";
        default:
            return std::to_string(sched_policy);
    }
}

}

#endif
