#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <mutex>
#include <condition_variable>

#include "util/circularbuffer.h"

// queuebuf's must be 'stop'ed before their owning processes are.

#include <util/exceptdef.h>
declareerror(queueerror)

// queuebuf is a queue meant to change the message rate, it can split or group data.
// To use it as a one-to-one queue, use the read(), write(T)  interface.
template<class T>
class queuebuf {
public:

    typedef T value_type;

    queuebuf(size_t size, const char*name) : _buf(size,name), _stopped(false), _nwrites(0), _nreads(0)
    {
#ifdef SAVE_QDATA
        char queuefile[256];
        strcpy(queuefile, name);
        strcat(queuefile, ".qdata");
        _f=fopen(queuefile, "wb");
#endif
    }
    ~queuebuf() {
#ifdef SAVE_QDATA
        fclose(_f);
#endif
        if (!_stopped)
            logmsg("WARNING: queue %s not stopped\n", _buf.name());
        logmsg("queue-%s : write=%d, read=%d\t", _buf.name(), _nwrites, _nreads);
    }

    // interface for shared_ptr items
    T read()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        while (!_stopped && _buf.usedsize()<1)
            _condrd.wait(lock);
        if (_stopped)
            throw queueerror("stopped");
        T data = _buf.read();
        _nreads ++;

        lock.unlock();
        _condwr.notify_one();
        return data;
    }

    void read(T* data, size_t size)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        while (!_stopped && _buf.usedsize()<size)
            _condrd.wait(lock);
        if (_stopped)
            throw queueerror("stopped");
        _buf.read(data, size);
        _nreads += size;

        lock.unlock();
        _condwr.notify_one();
    }
    // used for reading a variable amount of data.
    // to be able to maximize the amount of data transmitted
    // in a packet.
    size_t read(T* data, size_t minsize, size_t maxsize)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        while (!_stopped && _buf.usedsize()<minsize)
            _condrd.wait(lock);
        if (_stopped)
            throw queueerror("stopped");
        size_t wantedsize= std::min(maxsize, _buf.usedsize());
        _buf.read(data, wantedsize);
        _nreads += wantedsize;

        lock.unlock();
        _condwr.notify_one();

        return wantedsize;
    }

    template<class V>
    void readvec(V& vec, size_t size)
    {
        vec.resize(size);
        read(&vec[0], vec.size());
    }

    // for reading the queue contents after it was stopped
    template<class V>
    void readbuf(V& vec)
    {
        vec.resize(_buf.usedsize());
        _buf.read(&vec[0], vec.size());
    }

    // interface for shared_ptr items
    void write(T data)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        while (!_stopped && _buf.freesize()<1)
            _condwr.wait(lock);
        if (_stopped)
            throw queueerror("stopped");
        _buf.write(data);
        _nwrites ++;

        lock.unlock();
        _condrd.notify_one();
    }
    void write(const T* data, size_t size)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        if (_buf.maxsize()<size)
            throw "queuebuf: datasize too large for buffer";
        while (!_stopped && _buf.freesize()<size)
            _condwr.wait(lock);
        if (_stopped)
            throw queueerror("stopped");
        _buf.write(data, size);
        _nwrites += size;

        lock.unlock();
        _condrd.notify_one();
#ifdef SAVE_QDATA
        fwrite(data, sizeof(T), size, _f);
#endif
    }
    void writeone(T v)
    {
        write(&v, 1);
    }
    template<class V>
    void writevec(const V& vec)
    {
        write(&vec[0], vec.size());
    }
    void stop()
    {
        if (_stopped)
            return;
        std::unique_lock<std::mutex> lock(_mtx);
        _stopped= true;
        lock.unlock();
        _condrd.notify_one();
        _condwr.notify_one();
    }

//    std::mutex& mutex() { return _qmtx; }
    size_t usedsize() const { return _buf.usedsize(); }
    size_t freesize() const { return _buf.freesize(); }
private:
    circularbuffer<T> _buf;
    std::condition_variable _condrd;
    std::condition_variable _condwr;
    std::mutex _mtx;
    bool _stopped;
    int _nwrites;
    int _nreads;

#ifdef SAVE_QDATA
    FILE *_f;
#endif
};

typedef std::vector<short> SampleVector;
//typedef std::vector<unsigned char> ByteVector;

typedef queuebuf<unsigned char> ByteQueue;
typedef queuebuf<short> SampleQueue;

#endif
