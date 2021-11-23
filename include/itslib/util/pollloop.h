#pragma once
#include <poll.h>
#include <memory>
#include "err/posix.h"
struct pollloop {

    struct callback {
        virtual ~callback() { }
        virtual void handleevent(int)= 0;
    };
    template<typename F>
    struct fncallback : callback {
        F f;
        fncallback(F f)
            : f(f)
        {
        }
        virtual void handleevent(int ev)
        {
            f(ev);
        }
    };

    template<typename F>
    static std::shared_ptr<callback> newhandler(F f)
    {
        return std::shared_ptr<callback>(new fncallback<F>(f));
    }

    // these 2 lists are kept in sync: elements with equal index correspond to each other
    std::vector<pollfd> pfd;
    std::vector<std::shared_ptr<callback> > handlers;


    static pollfd makepollfd(int fd, int flags)
    {
        pollfd p;
        p.fd= fd;
        p.events= flags;
        p.revents= 0;
        return p;
    }
    void add(int fd, int flags, std::shared_ptr<callback> cb)
    {
        auto i= upper_bound(pfd.begin(), pfd.end(), fd, [](int fd, const pollfd& f){ return fd < f.fd; });
        // value >= [begin .. upperbound(begin,end,value) )
        // value > [begin .. lowerbound(begin,end,value) )  

        int ix= i-pfd.begin();

        if (i->fd==fd) {
            printf("poll: fd %d already there\n", fd);
            return;
        }

        pfd.insert(i, makepollfd(fd, flags));
        handlers.insert(handlers.begin()+ix, cb);
    }
    void remove(int fd)
    {
        auto i= upper_bound(pfd.begin(), pfd.end(), fd, [](int fd, const pollfd& f){ return fd < f.fd; });

        if (i==pfd.begin()) // list is empty
            return;
        i--;
        if (i->fd != fd)  // not found
            return;
        int ix= i-pfd.begin();

        pfd.erase(i);
        handlers.erase(handlers.begin()+ix);
    }

    void process()
    {
        int n=poll(&pfd.front(), pfd.size(), 1000);
        if (n==-1)
            throw posixerror("poll");

        if (n) {
            auto h= handlers.begin();
            for (auto i= pfd.begin() ; i!=pfd.end() ; ++i, ++h)
                if (i->revents) {
                    (*h)->handleevent(i->revents);
                }
        }
    }

    int count() const
    {
        return pfd.size();
    }
};

