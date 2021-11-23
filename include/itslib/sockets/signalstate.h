#ifndef _SOCK_SIGNALSTATE_H_
#define _SOCK_SIGNALSTATE_H_
#include "util/logmsg.h"
#include "sockets/socketstate.h"
#include "sockets/tcpsocket.h"
#ifdef _WIN32
class signalstate : public socketstate {
    tcpsocket_ptr _a;
    tcpsocket_ptr _c;
    int _verbose;
public:
    signalstate()
        : _c(new tcpsocket()), _verbose(0)
    {
        tcpsocket _l;
        _l.listen(tcpaddress("127.0.0.1"));

        _c->connect(_l.getsock());
        _a= _l.accept();

        _a->setnonblocking();
        _c->setnonblocking();

        ev_connected();

        _desc += stringformat("signal %d:%s -> %d:%s\n", _a->fd(), _a->getsockname().c_str(), _c->fd(), _c->getsockname().c_str());
        logmsg("%s %s\n", logstamp().c_str(), _desc.c_str());
    }

    void ack()
    {
        uint8_t c= 0x55;
        _c->read(&c, 1);
        if (_verbose > 1)
            loginfo("%s acked: %02x\n", logstamp().c_str(), c);
    }
    void notify()
    {
        uint8_t c= 0xaa;
        _a->write(&c, 1);
        if (_verbose > 1)
            loginfo("%s notified: %d: %02x\n", logstamp().c_str(), _a->fd(), c);
    }

    virtual void mayread() { ack(); }
    virtual void maywrite() { }

    virtual bool needs(int need)
    {
        switch(_state) {
            case NEW: return false;
            case DISCONNECTED: return false;
            case FAILED: return false;
            default:
                return need==NEED_RD;
        }
    }
    virtual int fd() { return _c->fd(); }
    virtual bool eof() { return false; }

    virtual size_t sockread(uint8_t *p, size_t nreq) { return 0; }
    virtual size_t sockwrite(const uint8_t *p, size_t nreq) { return 0; }
}
;
#else
class signalstate : public socketstate {
    tcpsocket_ptr _a;
    tcpsocket_ptr _c;
    int _verbose;
public:
    signalstate()
        : _verbose(0)
    {
        _desc = "signal";
        int fds[2];
        if (-1==socketpair(AF_UNIX, SOCK_STREAM, 0, fds))
            throw "socketpair";
        _a= tcpsocket_ptr(new tcpsocket(fds[0]));
        _c= tcpsocket_ptr(new tcpsocket(fds[1]));

        _a->setnonblocking();
        _c->setnonblocking();

        ev_connected();

        _desc += stringformat(" %d -> %d", _a->fd(), _c->fd());
        logmsg("%s %s\n", logstamp().c_str(), _desc.c_str());
    }

    void ack()
    {
        uint8_t c= 0x55;
        _c->read(&c, 1);
        if (_verbose > 1)
            loginfo("%s acked: %02x\n", logstamp().c_str(), c);
    }
    void notify()
    {
        uint8_t c= 0xaa;
        _a->write(&c, 1);
        if (_verbose > 1)
            loginfo("%s notified: %02x\n", logstamp().c_str(), c);
    }

    virtual void mayread() { ack(); }
    virtual void maywrite() { }

    virtual bool needs(int need)
    {
        return need==NEED_RD;
    }
    virtual int fd()
    {
        return _c->fd();
    }
    virtual bool eof() { return false; }
    virtual size_t sockread(uint8_t *p, size_t nreq) { return 0; }
    virtual size_t sockwrite(const uint8_t *p, size_t nreq) { return 0; }
};
#endif
#endif
