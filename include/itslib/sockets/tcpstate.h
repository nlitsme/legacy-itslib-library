#ifndef _SOCK_TCPSTATE_H_
#define _SOCK_TCPSTATE_H_
#include "util/logmsg.h"
#include "util/HiresTimer.h"
#include "sockets/socketstate.h"
#include "sockets/tcpsocket.h"

class tcpstate : public socketstate {
protected:
    tcpsocket_ptr _s;
    HiresTimer _tstart;
    HiresTimer _tconnected;

    tcpaddress _target;
public:
    tcpstate(const tcpaddress& target)
        : _s(new tcpsocket()), _target(target)
    {
        _s->setnonblocking();
        _desc= stringformat("sock %d", _s->fd());
        if (_verbose>1)
            logprogress("%s created %s\n", logstamp().c_str(), _desc.c_str());
    }
    tcpstate(tcpsocket_ptr s)
        : _s(s)
    {
        ev_connected();
        _desc= stringformat("asock %d %s<-%s", _s->fd(), s->getsockname().c_str(), s->getpeername().c_str());
        logprogress("%s accepted %s\n", logstamp().c_str(), _desc.c_str());
    }
    ~tcpstate()
    {
        _s.reset();
    }
    virtual void start(socket_ptr discard)
    {
        socketstate::start(discard);

        logprogress("%s tcpconnecting %s\n", logstamp().c_str(), _desc.c_str());
        _s->connect(_target);
    }

    void listen(const tcpaddress& addr)
    {
        _state= LISTENING;
        _desc= stringformat("isock %d %s", _s->fd(), addr.asstring().c_str());
        logprogress("%s tcplistening %s\n", logstamp().c_str(), _desc.c_str());
        _s->listen(addr);
    }
    virtual void close()
    {
        socketstate::close();
        if (_s) {
            _s->shutdown();
            _s->close();
        }
        _s.reset();
    }

    virtual void mayread()
    {
        if (_state==CONNECTING) {
            printf("mayread while tcpconnecting -> fail  - %s\n", _desc.c_str());
            fail();
        }
        else if (_state==LISTENING) {
            tcpsocket_ptr a= _s->accept();
            a->setnonblocking();
            ev_tcpaccept(a);
        }
        else if (_state==CONNECTED) {
            ev_sockread();
        }
    }
    virtual void maywrite()
    {
        if (_state==CONNECTING) {
            int status= _s->getsocketerror();
            if (status) {
                printf("socketerror %d -> fail\n", status);
                fail();
            }
            else {
                ev_connected();
            }
        }
        else if (_state==LISTENING) {
            printf("maywrite while listening -> fail\n");
            fail();
        }
        else if (_state==CONNECTED) {
            ev_sockwrite();
        }
    }

    virtual void ev_tcpaccept(tcpsocket_ptr a)
    {
        // todo - add to selectloop
        logprogress("%s %s tcp accept\n", logstamp().c_str(), _desc.c_str());
    }
    virtual int fd() { return _s->fd(); }
    virtual bool eof() { return _s->eof(); }

    // sockread+sockwrite are overridden in sslstate
    virtual size_t sockread(uint8_t *p, size_t nreq)
    {
        return _s->read(p, nreq);
    }
    virtual size_t sockwrite(const uint8_t *p, size_t nreq)
    {
        return _s->write(p, nreq);
    }

    // needs is overridden in ssl, where it always returns true
    virtual bool needs(int need)
    {
        switch(_state) {
            case CONNECTING: return need==NEED_WR;
            case LISTENING: return need==NEED_RD;
            case NEW: return false;
            case DISCONNECTED: return false;
            case FAILED: return false;
            default: 
                   if (need==NEED_RD)
                       return !_s->eof();
                   else
                       return !_s->eof() && _outuq.usedsize()>0;
        }
    }
};
#endif
