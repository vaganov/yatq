#ifndef _YATQ_UTILS_SCHED_UTILS_H
#define _YATQ_UTILS_SCHED_UTILS_H

#include <cerrno>
#include <string>

#include <pthread.h>

#include "yatq/internal/log4cxx_proxy.h"
#include "yatq/utils/logging_utils.h"

namespace yatq::utils {

typedef enum {min_priority = 0, max_priority = -1} priority_t;

inline bool set_sched_params(pthread_t handle, int sched_policy, int priority, const std::string& thread_tag = "unspecified") {
#ifndef YATQ_DISABLE_LOGGING
    static auto logger = log4cxx::Logger::getLogger("yatq.utils.sched");
#endif

    int prev_sched_policy;  // Linux implementation crashes on 'nullptr'
    sched_param sched;
    pthread_getschedparam(handle, &prev_sched_policy, &sched);
    sched.sched_priority = priority;
    if (pthread_setschedparam(handle, sched_policy, &sched) == 0) {
        LOG4CXX_INFO(logger, "Set sched params thread=" + thread_tag + " policy=" + sched_policy_to_string(sched_policy) + " priority=" + std::to_string(priority));
        return true;
    }
    else {
        LOG4CXX_WARN(logger, "Failed to set sched params thread=" + thread_tag + ": " + std::string(std::strerror(errno)));
        return false;
    }
}

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
