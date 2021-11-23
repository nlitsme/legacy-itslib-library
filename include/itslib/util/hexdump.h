#ifndef __HEXDUMP_H__
#define __HEXDUMP_H__
#include <iostream>
template<typename T>
void hexdump(std::ostream& os, const T*p, size_t n)
{
    if (sizeof(T)==1 || sizeof(T)==2 || sizeof(T)==4)
    {
        std::string buf; buf.resize(n*(sizeof(T)*2+1));
        for (size_t i=0 ; i<n ; i++)
        {
            if (i)
                buf[i*(sizeof(T)*2+1)-1]=' ';
            sprintf(&buf[i*(sizeof(T)*2+1)], "%0*x", (int)sizeof(T)*2, p[i]);
        }
        // remove terminating NUL
        buf.resize(buf.size()-1);
        os << buf;
    }
}
#endif
