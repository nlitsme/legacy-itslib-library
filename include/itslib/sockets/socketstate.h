#ifndef _SOCK_SOCKETSTATE_H_
#define _SOCK_SOCKETSTATE_H_

#include <list>
#include <memory>
#include <cassert>
#include "util/logmsg.h"
#include "util/queuebuf.h"
#include "util/ungetqueue.h"

// once an object is 'connected' it deletes itself from the selectlist,
// and hand control to it's 'next' item

enum need_t { NEED_RD, NEED_WR };

#define STFORWARDDECLARE(name) \
    class name##state; \
    typedef std::shared_ptr<name##state> name##_ptr;

#define STCONVENIENCECAST(name) \
    name##_ptr as_##name() { return std::dynamic_pointer_cast<name##state>(shared_from_this()); }


STFORWARDDECLARE(tcp)
#ifdef _SOCK_SSLSTATE_H_
STFORWARDDECLARE(ssl)
#endif
STFORWARDDECLARE(socks4)

// forward declaration
class socketstate;
typedef std::shared_ptr<socketstate> socket_ptr;

typedef ungetqueue<uint8_t> UngetByteQueue;


class socketstate : public std::enable_shared_from_this<socketstate> {
protected:
    int _state;
    socket_ptr  _next;
    HiresTimer _tstart;
    HiresTimer _tsession;
    uint64_t _duration_session;
    uint64_t _duration_connect;
    std::string _desc;

    ByteQueue _outq;
    UngetByteQueue _outuq;
    ByteQueue _inq;

    int _verbose;

    int _connecttimeout;
    int _sessiontimeout;
public:
enum state_t { NEW, CONNECTED, DISCONNECTED, FAILED, LISTENING, CONNECTING, ACCEPTING };
    socketstate()
        : _state(NEW), _duration_session(0), _duration_connect(0), _outq(4096, "outq"), _outuq(_outq), _inq(4096, "inq"), _verbose(0), _connecttimeout(5000), _sessiontimeout(5000)
    {
        _desc= "sock";
    }
    virtual ~socketstate()
    {
        _inq.stop();
        _outq.stop();
    }
    void next(socket_ptr n)
    {
        assert(_state==NEW);
        _next= n;
    }

    // interface for selectloop
    virtual void mayread()= 0;
    virtual void maywrite()= 0;

    bool candelete()
    {
        if (_state==FAILED || _state==DISCONNECTED) {
            logprogress("%s %s candelete\n", logstamp().c_str(), _desc.c_str());
            return true;
        }
        if (_state!=CONNECTED && _tstart.msecelapsed()>_connecttimeout) {
            logprogress("%s %s candelete - timeout\n", logstamp().c_str(), _desc.c_str());
            return true;
        }
        if (_state==CONNECTED && eof()) {
            logprogress("%s %s candelete - EOF\n", logstamp().c_str(), _desc.c_str());
            return true;
        }
        if (_state==CONNECTED && _next) {
            logprogress("%s %s candelete -> next\n", logstamp().c_str(), _desc.c_str());
            return true;
        }
//      if (_state==CONNECTED && session_time() > _sessiontimeout) {
//          logprogress("%s %s candelete - session timeout\n", logstamp().c_str(), _desc.c_str());
//          return true;
//      }
        return false;
    }
    std::string desc() const { return _desc; }


    // must be implemented by subclass
    // returns true when the specified 'need' is needed
    virtual bool needs(int need)= 0;

    // must be implemented by subclass:
    //   tcpstate, signalstate return the fd() of a tcpsocket_ptr
    //   while ssl, socks4 return the fd() of a socket_ptr
    virtual int fd()= 0;
    virtual bool eof()= 0;

    virtual void start(socket_ptr from)
    {
        _tstart.reset();
        _state= CONNECTING;
    }


    void fail()
    {
        _state= FAILED;
        if (_next)
            _next->fail();
    }

    void ev_connected()
    {
        _state= CONNECTED;
        _duration_connect= _tstart.elapsed();
        _tsession.reset();

        if (_next)
            _next->start(shared_from_this());
    }
    int connect_time() { return _duration_connect; }
    int session_time() { return _state==CONNECTED ? _tsession.elapsed() : _duration_session; }

    // external interface to send data to / from this socket
    size_t read(uint8_t *p, size_t nreq)
    {
        if (_state==DISCONNECTED)
            throw "read:disconnected";
        size_t want= std::min(nreq, _inq.usedsize());
        _inq.read(p, want);
        return want;
    }
    size_t write(const uint8_t *p, size_t nreq)
    {
        size_t want= std::min(nreq, _outq.freesize());
        _outq.write(p, want);
        // todo: bump selectloop signal so we get added to the 'want-to-write list again
        //printf("%s %s todo: bump for write\n", logstamp().c_str(), _desc.c_str());
        return want;
    }

    size_t write(const std::string& str)
    {
        if (_state==DISCONNECTED)
            throw "write:disconnected";
        return write((const uint8_t*)str.c_str(), str.size());
    }


    // called to move data to/from socket <-> queue
    void ev_sockread()
    {
        if (eof() || _inq.freesize()==0) {
            logprogress("%s %s r:DISCONNECTED\n", logstamp().c_str(), _desc.c_str());
            _state= DISCONNECTED;
            _duration_session = _tsession.elapsed();
            return;
        }

        ByteVector data(_inq.freesize());
        size_t n= sockread(&data[0], data.size());
        assert(n<=data.size());

        data.resize(n);
        if (_verbose && n)
            printf("%s %s read %d bytes:\n%s\n", logstamp().c_str(), _desc.c_str(), (int)n, ascdump(data, "", true).c_str());

        _inq.write(&data[0], n);
    }
    void ev_sockwrite()
    {
        if (eof()) {
            logprogress("%s %s w:DISCONNECTED\n", logstamp().c_str(), _desc.c_str());
            _state= DISCONNECTED;
            _duration_session = _tsession.elapsed();
            return;
        }

        ByteVector data(_outuq.usedsize());
        if (!data.empty()) {
            _outuq.readbytes(&data[0], data.size());
            size_t n= sockwrite(&data[0], data.size());
            assert(n<=data.size());

            if (n<data.size())
                _outuq.ungetbytes(&data[n], data.size()-n);
            loginfo("%s %s wrote %d of %d bytes: %s\n", logstamp().c_str(), _desc.c_str(), (int)n, (int)data.size(), hexdump(&data[0], n).c_str());
        }
        else {
            //logmsg("%s %s nothing to write\n", logstamp().c_str(), _desc.c_str());
        }
    }
    // low level socket access
    virtual size_t sockread(uint8_t *p, size_t nreq)= 0;
    virtual size_t sockwrite(const uint8_t *p, size_t nreq)= 0;

    void verbose(int n) { _verbose= n; }
    void connecttimeout(unsigned n) { _connecttimeout= n; }
    void sessiontimeout(unsigned n) { _sessiontimeout= n; }

    virtual void close()
    { 
        _state= DISCONNECTED;
        _duration_session = _tsession.elapsed();
        _inq.stop();
        _outq.stop();
//      if (_next)
//          _next->close();
    }
    bool isdisconnected() const { return _state==DISCONNECTED; }
    // for reading the queue's contents after disconnect
    void readbuf(ByteVector& data)
    {
        _inq.readbuf(data);
    }
STCONVENIENCECAST(tcp)
#ifdef _SOCK_SSLSTATE_H_
STCONVENIENCECAST(ssl)
#endif
STCONVENIENCECAST(socks4)

};
typedef std::list<socket_ptr> socketlist;
#endif
