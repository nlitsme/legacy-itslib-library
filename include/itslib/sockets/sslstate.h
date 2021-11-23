#ifndef _SOCK_SSLSTATE_H_
#define _SOCK_SSLSTATE_H_
#include "util/logmsg.h"
#include "sockets/socketstate.h"
#include "sockets/sslsocket.h"
class sslstate : public socketstate {
    socket_ptr _s;

    sslcontext_ptr _ctx;
    sslsocket_ptr _ssl;

public:
    sslstate(sslcontext_ptr ctx)
        : _ctx(ctx)
    {
        _desc= stringformat("sslout");
        logprogress("%s %s ssl created\n", logstamp().c_str(), _desc.c_str());
    }
    sslstate(sslcontext_ptr ctx, socket_ptr s)
        : _s(s), _ctx(ctx), _ssl(_ctx->newsocket(s->fd()))
    {
        _desc= stringformat("sslin");
        logprogress("%s %s ssl accepting\n", logstamp().c_str(), _desc.c_str());
        _ssl->setnonblocking();
        _state= ACCEPTING;
    }
    virtual void mayread()
    {
        //printf("r:%d\n", _state);
        if (_state==CONNECTING)
        {
            handle_ssl_connect();
        }
        else if (_state==ACCEPTING)
        {
            if (_ssl->accept())
                ev_connected();
        }
        else if (_state==CONNECTED)
        {
            ev_sockread();
        }
    }
    virtual void maywrite()
    {
        //printf("r:%d\n", _state);
        if (_state==CONNECTING)
        {
            handle_ssl_connect();
        }
        else if (_state==ACCEPTING)
        {
            if (_ssl->accept())
                ev_connected();
        }
        else if (_state==CONNECTED)
        {
            ev_sockwrite();
        }
    }
    void handle_ssl_connect()
    {
        if (_ssl->connect()) {
            printf("C\n");
            ev_connected();
        }
        else {
            printf("c\n");
        }
    }

    virtual void start(socket_ptr s)
    {
        _s= s;

        _desc += "(" + _s->desc() + ")";

        _ssl= _ctx->newsocket(s->fd());
        _ssl->setnonblocking();

        logprogress("%s %s ssl connecting\n", logstamp().c_str(), _desc.c_str());
        socketstate::start(s);
        handle_ssl_connect();
    }
    virtual int fd() { return _s ? _s->fd() : -1; }
    virtual bool eof() { return _s->eof(); }

    virtual size_t sockread(uint8_t *p, size_t nreq)
    {
        int n= _ssl->read(p, nreq);
        if (n==-1) // want more
            return 0;
        return n;
    }
    virtual size_t sockwrite(const uint8_t *p, size_t nreq)
    {
        int n= _ssl->write(p, nreq);
        if (n==-1) // want more
            return 0;
        return n;
    }
    virtual bool needs(int need)
    {
        switch(_state) {
            case NEW: return false;
            case DISCONNECTED: return false;
            case FAILED: return false;
            case CONNECTING: return need==NEED_RD;// it appears to be sufficient to just 'rd'
            case ACCEPTING: return true;
            default: 
                   if (need==NEED_RD)
                       return !_s->eof();
                   else
                       return !_s->eof() && _outuq.usedsize()>0;

        }
        // todo: ssl api returns specific want read/write status codes
    }


};
#endif
