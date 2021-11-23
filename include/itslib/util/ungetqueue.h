#ifndef _ENGINE_UTIL_UNGETQUEUE_H_
#define _ENGINE_UTIL_UNGETQUEUE_H_

#include "util/queuebuf.h"

#define ungettrace(...)

// <queue> [buf]  -> read
template<typename T>
class ungetqueue {
    typedef queuebuf<T> queuetype;
    typedef std::vector<T> vectortype;

    queuetype& _q;
    vectortype _ungetbuf;

    size_t _ungetptr;  // when ungetptr==_ungetbuf.size() -> read from queue
public:
    ungetqueue(queuetype& q)
        : _q(q), _ungetptr(0)
    {
    }
    void stop()
    {
        _q.stop();
    }

    T readbyte()
    {
        if (_ungetptr<_ungetbuf.size()) {
            return _ungetbuf[_ungetptr++];
        }
        else {
            uint8_t byte;
            _q.read(&byte, 1);

            return byte;
        }
    }
    void readbytes(T *p, size_t n)
    {
        if (n>0 && _ungetptr<_ungetbuf.size()) {
            size_t want= std::min(_ungetbuf.size()-_ungetptr, n);
            std::copy(&_ungetbuf[_ungetptr], &_ungetbuf[_ungetptr]+want, p);
            ungettrace("unget: read %d bytes from ungetbuf: %s\n", want, hexdump((const uint8_t*)p, want*sizeof(T)).c_str());
            p += want;
            _ungetptr += want;
            n -= want;
        }
        if (n) {
            ungettrace("unget: reading %d bytes from queue (%d)\n", n, _q.usedsize());
            _q.read(p, n);
            ungettrace("    unget queue: %s\n", hexdump((const uint8_t*)p, n*sizeof(T)).c_str());
            p+= n;
            n= 0;
        }
    }
    void ungetbytes(const uint8_t *p, size_t n)
    {
        if (_ungetptr<n) {
            ungettrace("ungetqueue: extending buffer from %d to %d bytes\n", _ungetbuf.size(), _ungetbuf.size()-_ungetptr+n);
            size_t orgsize= _ungetbuf.size();
            _ungetbuf.resize(_ungetbuf.size()-_ungetptr+n);

            ungettrace("unget: shifting data to end\n");
            std::copy_backward(&_ungetbuf[_ungetptr], &_ungetbuf[orgsize], _ungetbuf.end());

            _ungetptr += _ungetbuf.size()-orgsize;
        }
        
        std::copy(p, p+n, &_ungetbuf[_ungetptr-n]);
        _ungetptr -= n;
    }

    // return nr of bytes that can be read at once right now.
    size_t usedsize()
    {
        return _ungetbuf.size()-_ungetptr + _q.usedsize();
    }

};
#endif
