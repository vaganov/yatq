#ifndef _YATQ_INTERNAL_LOG4CXX_PROXY_H
#define _YATQ_INTERNAL_LOG4CXX_PROXY_H

#ifndef YATQ_DISABLE_LOGGING

#include <log4cxx/logger.h>
#include <log4cxx/mdc.h>

#define SET_THREAD_TAG(tag) log4cxx::MDC::put("threadTag", tag)

#else

#define LOG4CXX_TRACE(...)
#define LOG4CXX_DEBUG(...)
#define LOG4CXX_INFO(...)
#define LOG4CXX_WARN(...)
#define LOG4CXX_ERROR(...)

#define SET_THREAD_TAG(...)

#endif

#endif
