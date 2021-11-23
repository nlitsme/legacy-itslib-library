#ifndef _SOCK_SELECTLOOP_H_
#define _SOCK_SELECTLOOP_H_
#include "util/logmsg.h"
#include "util/process.h"
#include "sockets/socketstate.h"
#include "sockets/signalstate.h"
class selectloop : public process {
    std::mutex _listmtx;
    socketlist _list;
    std::shared_ptr<signalstate> _signal;
    int _itercount;
    int _pollinterval;
    int _verbose;
    bool _error;
public:
    selectloop()
        : _signal(new signalstate()), _itercount(0), _pollinterval(1), _verbose(0), _error(false)
    {
        add(_signal);
        start();
    }
    virtual ~selectloop()
    {
        stop();
    }
    virtual void processstop()
    {
        _signal->notify();
    }
    virtual void handle_error()
    {
        logerror("%s EXCEPTION in selectloop\n", logstamp().c_str());
        _error= true;
    }
    bool haveerror() const { return _error; }
    virtual const char*name() { return "selectloop"; }
    virtual void service()
    {
        sleep(1);
    }
    virtual void servicexx()
    {
        fd_set wr; int mw= getlistbits(NEED_WR, &wr);
        fd_set rd; int mr= getlistbits(NEED_RD, &rd);

        timeval to;
        to.tv_sec= _pollinterval;
        to.tv_usec= 0;

        if (_verbose > 1)
            loginfo("\nselect(%d, [%s], [%s])\n", std::max(mw,mr)+1, fdsetstring(&rd).c_str(), fdsetstring(&wr).c_str());
        int n= select(std::max(mw,mr)+1, &rd, &wr, 0, &to);
        if (n<0) {
            printf("\nselect(%d, [%s], [%s])\n", std::max(mw,mr)+1, fdsetstring(&rd).c_str(), fdsetstring(&wr).c_str());
            _error= true;
            throw socketerror("select");
        }
        if (_verbose > 1)
            loginfo("select->%d\n", n);

        if (isterminating())
            return;

        int act=0;
        int removed=0;
        int remaining=0;

        socketlist::iterator i=_list.begin();
        while (i!=_list.end())
        {
            socket_ptr s= *i;

            try {
            if (isinbits(s, &rd)) {
                s->mayread();
                act++;
            }

            if (isinbits(s, &wr)) {
                s->maywrite();
                act++;
            }
            }
            catch(...)
            {
                s->fail();
            }

            if (s->candelete()) {
                std::unique_lock<std::mutex> lock(_listmtx);
                _list.erase(i++);
                removed++;
            }
            else {
                i++;
                remaining++;
            }
        }
//      if (removed || remaining || act)
//          printf("selectloop: removed=%d, remaining=%d, act=%d\n",
//                  removed, remaining, act);

        if (act==0)
            usleep(100000);
    }
    std::string fdsetstring(fd_set *s)
    {
        std::string str= "";
        for (int i=0 ; i<1024 ; i++)
            if (FD_ISSET(i, s)) {
                if (!str.empty())
                    str+=",";
                str += stringformat("%d", i);
            }
        return str;
    }
    int getlistbits(int need, fd_set *fs)
    {
        int count=0;
        int m=0;
        FD_ZERO(fs);
        for (socketlist::iterator i=_list.begin() ; i!=_list.end() ; ++i) {
            socket_ptr s= *i;
            if (s->needs(need)) {
                FD_SET(s->fd(), fs);
                count++;
                if (m < s->fd())
                    m= s->fd();
            }
        }
        return m;
    }
    bool isinbits(socket_ptr s, fd_set *fs)
    {
        int fd= s->fd();
        if (fd<0)
            return false;
        return FD_ISSET(fd, fs);
    }

    void add(socket_ptr s)
    {
        std::unique_lock<std::mutex> lock(_listmtx);
        _list.push_back(s);
        lock.unlock();

        // bump select
        _signal->notify();
    }
    void bump()
    {
        _signal->notify();
    }
    size_t count() { return _list.size()-1; }

    void verbose(int n) { _verbose= n; }
    void pollinterval(int n) { _pollinterval= n; bump(); }

    void expiresessions(int sessiontimeout)
    {
        int expired=0;
        int remaining=0;
        for (auto i= _list.begin() ; i!=_list.end() ; )
        {
            if ((*i)->isdisconnected() || (*i)->session_time()/1000 > sessiontimeout) {
                (*i)->close();
                ByteVector data;
                (*i)->readbuf(data);
                if (data.size() || (*i)->session_time())
                    printf("\n%s %s read %d bytes\n%s\n", logstamp().c_str(), (*i)->desc().c_str(), (int)data.size(), ascdump(data, "", true).c_str());

                _list.erase(i++);
                expired++;
            }
            else {
                ++i;
                remaining++;
            }
        }
        //printf("expiry: %d expired, %d remaining\n", expired, remaining);
    }

};


#endif
