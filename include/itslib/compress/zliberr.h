#ifndef _COMPRESS_LZIBERROR_H_
#define _COMPRESS_LZIBERROR_H_
#include <string>
struct zliberror {
    int stat;
    std::string msg;
    zliberror(int stat, const char*msg)
        : stat(stat), msg(msg)
    {
    }
    ~zliberror()
    {
        printf("ZLIB[%d]: %s\n", stat, msg.c_str());
    }
};
#endif
