#ifndef _UTIL_EXCEPTDEF_H_
#define _UTIL_EXCEPTDEF_H_

#include "util/logmsg.h"

#ifdef _WIN32
#include <windows.h>
#define declareerror(n) class n { const char*_msg; public: n(const char*msg) : _msg(msg) { } ~n() { logmsg("@%08lx[cpip]" #n " - %s\n", GetTickCount(), _msg); } };
#else
#define declareerror(n) class n { const char*_msg; public: n(const char*msg) : _msg(msg) { } ~n() { logmsg("@%08lx[cpip]" #n " - %s\n", clock(), _msg); } };
#endif

#endif
