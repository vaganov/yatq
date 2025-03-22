#ifndef _YATQ_UTILS_SCHED_UTILS_H
#define _YATQ_UTILS_SCHED_UTILS_H

#include <cerrno>
#include <format>
#include <string>

#include <pthread.h>

#include "yatq/internal/log4cxx_proxy.h"
#include "yatq/utils/logging_utils.h"

namespace yatq::utils {

typedef enum {min_priority = 0, max_priority = -1} priority_t;

/**
 * set scheduling parameters for a thread
 * @param handle thread handle
 * @param sched_policy \a SCHED_OTHER | \a SCHED_RR | \a SCHED_FIFO
 * @param priority explicit priority
 * @param thread_tag thread tag (used for logging purposes only)
 * @return \a true if scheduling parameters have been set and \a false otherwise
 */
inline bool set_sched_params(pthread_t handle, int sched_policy, int priority, const std::string& thread_tag = "unspecified") {
#ifndef YATQ_DISABLE_LOGGING
    static auto logger = log4cxx::Logger::getLogger("yatq.utils.sched");
#endif

    int prev_sched_policy;  // Linux implementation crashes on 'nullptr'
    sched_param sched;
    pthread_getschedparam(handle, &prev_sched_policy, &sched);
    sched.sched_priority = priority;
    if (pthread_setschedparam(handle, sched_policy, &sched) == 0) {
        LOG4CXX_INFO(
            logger,
            std::format(
                "Set sched params thread='{}' policy={} priority={}",
                thread_tag, sched_policy_to_string(sched_policy), priority
            )
        );
        return true;
    }
    else {
        LOG4CXX_WARN(logger, std::format("Failed to set sched params thread='{}': {}", thread_tag, std::strerror(errno)));
        return false;
    }
}

/**
 * set scheduling parameters for a thread
 * @param handle thread handle
 * @param sched_policy \a SCHED_OTHER | \a SCHED_RR | \a SCHED_FIFO
 * @param priority_tag \a yatq::utils::max_priority | \a yatq::utils::min_priority
 * @param thread_tag thread tag (used for logging purposes only)
 * @return \a true if scheduling parameters have been set and \a false otherwise
 */
inline bool set_sched_params(pthread_t handle, int sched_policy, priority_t priority_tag, const std::string& thread_tag = "unspecified") {
    int priority;
    switch (priority_tag) {
        case min_priority:
            priority = sched_get_priority_min(sched_policy);
            break;
        case max_priority:
            priority = sched_get_priority_max(sched_policy);
            break;
    }
    return set_sched_params(handle, sched_policy, priority, thread_tag);
}

}

#endif
