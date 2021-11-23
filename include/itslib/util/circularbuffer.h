#ifndef __CIRCULARBUFFER_H__
#define __CIRCULARBUFFER_H__

#include <algorithm>
#include <vector>
#include "util/hexdump.h"
#include "util/logmsg.h"

#include <util/exceptdef.h>

#ifdef DUMP_CB_DATA
#include <mutex>
#endif
declareerror(buffererror)

template<class T>
class circularbuffer {
public:
    circularbuffer(size_t bufsize, const char*name) : _name(name), _size(0), _rdptr(0), _wrptr(0), _buf(bufsize)
    {
    }
    ~circularbuffer()
    {
        logmsg("at exit: %u/%u/%u %s\n", (unsigned)_size, (unsigned)_rdptr, (unsigned)_wrptr, _name);
    }

    // interface for shared_ptr items
    T read()
    {
        if (1>usedsize())
            throw buffererror("buffer underflow");
        T data= _buf[_rdptr];
        _rdptr++; if (_rdptr==_buf.size()) _rdptr= 0;
        _size--;

        return data;
    }
    void read(T* data, size_t size)
    {
        if (size>usedsize())
            throw buffererror("buffer underflow");
        size_t copied=0;
        while (copied<size)
        {
            size_t wanted= std::min(_buf.size()-_rdptr, size-copied);
            memcpy(&data[copied], &_buf[_rdptr], wanted*sizeof(T));
            _rdptr+=wanted; if (_rdptr==_buf.size()) _rdptr= 0;
            _size-=wanted;
            copied+=wanted;
        }
#ifdef DUMP_CB_DATA
        logdata(_name, "read", data, size);
#endif
    }

    // interface for shared_ptr items
    void write(T data)
    {
        if (1>freesize())
            throw buffererror("buffer overflow");

        _buf[_wrptr]= data;
        _wrptr++; if (_wrptr==_buf.size()) _wrptr= 0;
        _size++;
    }

    void write(const T* data, size_t size)
    {
        if (size>freesize())
            throw buffererror("buffer overflow");
#ifdef DUMP_CB_DATA
        logdata(_name, "writ", data, size);
#endif
        size_t copied=0;
        while (copied<size)
        {
            size_t wanted= std::min(_buf.size()-_wrptr, size-copied);
            memcpy(&_buf[_wrptr], &data[copied], wanted*sizeof(T));
            _wrptr+=wanted; if (_wrptr==_buf.size()) _wrptr= 0;
            _size+=wanted;
            copied+=wanted;
        }
    }
    size_t usedsize() const
    {
        return _size;
    }
    size_t freesize() const
    {
        return _buf.size()-_size;
    }
    size_t maxsize() const
    {
        return _buf.size();
    }
    const char*name() const { return _name; }

private:
    const char *_name;
    size_t _size;
    size_t _rdptr;
    size_t _wrptr;
    std::vector<T> _buf;

#ifdef DUMP_CB_DATA
void logdata(const char*name, const char*msg, const T* data, size_t size)
{
    return;
    static std::mutex g_coutmtx;
    std::unique_lock<std::mutex> lock(g_coutmtx);
    std::cout << "queue-" << name << ':' << msg << ':';
    hexdump(std::cout, data, size);
    std::cout << std::endl;
}
#endif

};
#endif
