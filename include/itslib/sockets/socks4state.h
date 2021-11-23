#ifndef _SOCK_SOCKS4STATE_H_
#define _SOCK_SOCKS4STATE_H_
#include "util/logmsg.h"
#include "sockets/socketstate.h"
#include "util/endianutil.h"

class socks4state : public socketstate {
    ByteVector _req;
    ByteVector _ans;

    socket_ptr  _s;
public:
    socks4state(const std::string& addr)
    {
        _desc= "socks4";
        makereqpacket(addr);
        write(&_req[0], _req.size());
    }
    virtual ~socks4state()
    {
    }

    virtual void close()
    { 
        socketstate::close();
        if (_s)
            _s->close();
        _s.reset();
    }
    bool parseaddr(const char*str, uint8_t *ip, uint16_t *port)
    {
        const char*p= &str[0];
        char *q=NULL;
        for (unsigned i=0 ; i<4 ; i++)
        {
            *ip++ = strtol(p, &q, 10);
            if (q==p)
                return false;
            if (i<3 && *q!='.')
                return false;
            p= q+1;
        }
        if (*q!=':')
            return false;
        p= q+1;
        *port= strtol(p, &q, 10);
        if (q==p)
            return false;
        return *q==0;
    }
    // note: not using tcpaddress, to prevent any name resolving
    void makereqpacket(const std::string& addr)
    {
        _ans.resize(8);
        _req.resize(9);

        _req[0]= 4;  // version
        _req[1]= 1;  // CONNECT

        uint16_t port;
        if (!parseaddr(addr.c_str(), &_req[4], &port)) {
            _req[4]= _req[5]= _req[6]= 0; _req[7]= 1;
            size_t icolon= addr.find(':');
            if (icolon==addr.npos)
                throw "invalid address format";
            port= strtol(&addr[icolon+1], 0, 10);

            _req.resize(10+icolon);
            std::copy(addr.c_str(), addr.c_str()+icolon, &_req[9]);
        }

        set16be(&_req[2], port);
        _req[8]= 0;  // userid

        _desc += stringformat(" %s", addr.c_str());
        logprogress("%s %s pkt: %s\n", logstamp().c_str(), _desc.c_str(), vhexdump(_req).c_str());
    }
    virtual void mayread()
    {
        if (_state==CONNECTING) {
            ev_sockread();
            handle_socks_connect();
        }
        else if (_state==CONNECTED) {
            ev_sockread();
        }
    }
    virtual void maywrite()
    {
        if (_state==CONNECTING) {
            ev_sockwrite();
        }
        else if (_state==CONNECTED) {
            ev_sockwrite();
        }
    }
    virtual void start(socket_ptr s)
    {
        _s= s;
        logprogress("%s %s socks connecting\n", logstamp().c_str(), _desc.c_str());
        socketstate::start(s);
        ev_sockread();
        ev_sockwrite();
        handle_socks_connect();
    }
    void handle_socks_connect()
    {
        if (_inq.usedsize()>=_ans.size()) {
            read(&_ans[0], _ans.size());

            if (_ans[0]!=0 || _ans[1]!=90) {
                logprogress("%s %s socks answer: %s\n", logstamp().c_str(), _desc.c_str(), vhexdump(_ans).c_str());
                fail();
            }
            else {
                uint16_t port= get16be(&_ans[2]);
                uint32_t ip= get32be(&_ans[4]);
                logprogress("%s %s socks to %08x : %04x\n", logstamp().c_str(), _desc.c_str(), ip, port);
                ev_connected();
            }
        }
    }
    virtual size_t sockread(uint8_t *p, size_t nreq) { return _s->sockread(p,nreq); }
    virtual size_t sockwrite(const uint8_t *p, size_t nreq) { return _s->sockwrite(p,nreq); }

    // needs is overridden in ssl, where it always returns true
    virtual bool needs(int need)
    {
        switch(_state) {
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
    virtual int fd() { return _s ? _s->fd() : -1; }
    virtual bool eof() { return _s->eof(); }

};


#endif
