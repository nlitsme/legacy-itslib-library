#ifndef __LOGMSG_H__
#define __LOGMSG_H__
#include "stringutils.h"

#ifdef _UNIX
#include <sys/time.h>
#include <stdio.h>
#endif

#if defined(_MSC_VER) || (__GNUC__<4) || defined(__clang__)
#define __disable_function__ while(0)
#else
// this avoids the useless expression with comma warning
#define __disable_function__ 
#endif


inline std::string logstamp()
{
#ifdef _WIN32
    SYSTEMTIME now;
    GetLocalTime(&now);
    DWORD tNow= GetTickCount();

    return stringformat("[%08lx:%08lx] %02d:%02d:%02d @%08lx", GetCurrentProcessId(), GetCurrentThreadId(),
            now.wHour, now.wMinute, now.wSecond, tNow);
#endif
#ifdef _UNIX
    struct timeval tv;
    gettimeofday(&tv, 0);
    struct tm tmlocal= *localtime(&tv.tv_sec);
    return stringformat("%02d:%02d:%02d.%06d", tmlocal.tm_hour, tmlocal.tm_min, tmlocal.tm_sec, tv.tv_usec);
#endif
    return "?";
}
#ifdef WITH_LOGGING
#include <stdarg.h>
inline void logmsg(const char*msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    //vprintf(msg, ap);
    printf("%s %llx %s", logstamp().c_str(), uint64_t(&msg)>>19, stringvformat(msg, ap).c_str());
    va_end(ap);
}
#elif defined(LOGMSG_DEBUG)
#include "debug.h"
#define logmsg debug
#else

#define logmsg(...) __disable_function__ 

#endif

#define logerror(...) logmsg(__VA_ARGS__)
#define loginfo(...) __disable_function__
#define logprogress(...) __disable_function__
#define logwarn(...) __disable_function__

#endif

