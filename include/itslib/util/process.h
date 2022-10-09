#ifndef __UTIL_PROCESS_H__
#define __UTIL_PROCESS_H__

#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <functional>

#include "util/logmsg.h"
#include "util/HiresTimer.h"
#include "util/atomicoperations.h"

#ifdef _WIN32
#include <windows.h>    // for GetCurrentThreadId
#endif

#include <map>
class process_exception { };
// subclasses of 'process' should call 'start()' in their constructor, and 'stop()' in their destructor.
class process {
public:
    process() : 
        _started(false),
        _terminatingself(false),
        _terminating(false), _stopped(false), _active(true)
                , _thread(std::ref(*this))
    {
        //loginfo("constructed thread %p\n", this);
    }
    virtual ~process() {
        // solve problem with exception in childclass constructor
        //   -> start() is not called, and operator() waits for that.
        if (!_started) {
            _terminating= true;
            _startcond.notify_one();
        }
        _thread.join();
        //loginfo("process destructed: %p\n", this);
        if (!_stopped)
            logerror("%p is not properly stopped\n", this);
    }
    virtual void service()= 0;
    virtual const char*name()=0;
    void start()
    {
        std::unique_lock<std::mutex> lock(_processmtx);
        //loginfo("proc started: %s\n", name());
        _started=true;
        lock.unlock();
        _startcond.notify_one();
    }
    // note: call 'stop' when another thread wants to stop this thread
    void stop()
    {
        //loginfo("stopping %p %s\n", this, name());
        if (!atomic_readwrite(&_terminating, true)) {
            _startcond.notify_one();

            try {
            processstop();
            }
            catch(...)
            {
                logerror("ERROR: exception in processstop()\n");
            }
        }
        else if (!_terminatingself) {
            logmsg("WARNING: 2 threads are trying to stop the same[%p] object\n", this);
        }
        wait_for_stop();
        //loginfo("wait_for_stop %s - done\n", name());
    }

    // note: call terminateservice when you want to terminate from within the service handler
    void terminateservice()
    {
        _terminatingself= true;
        //loginfo("terminateservice %p %s\n", this, name());
        atomic_readwrite(&_terminating, true);
    }
    // call this function from within a service() to determine if we need to stop
    bool isterminating() const
    {
        return atomic_read(const_cast<int32_t*>(&_terminating));
    }
    bool hasstopped() const
    {
        return _stopped;
    }
    // processstop can be overridden, to close sockets etc, 
    // so anything waiting in 'service' will exit/throw, so
    // operator()()  can continue, and detect the '_terminating' flag.
    // also, if your service() is waiting on some condition, notify
    // that condition here.
    virtual void processstop()
    {
    }
    virtual void at_service_start()
    {
        // put init which requires a fully initialized virtual object here
    }
    virtual void handle_error()
    {
    }
    void stopped()
    {
        std::unique_lock<std::mutex> lock(_processmtx);
        //loginfo("stopped %p %s\n", this, name());
        _stopped=true;
        lock.unlock();
        _stopcond.notify_all();
    }

    void operator()()
    {
        _active= true;
        // note: you cannot use 'name()' or other virtual methods until after wait_for_start()
        //loginfo("waitforstart %p\n", this);
        wait_for_start();
        if (_terminating || _stopped) {
            // can't use virtualmethods in this case, probably the destructor is waiting with thread.join
            logmsg("ERROR: failed to start %p\n", this);
            stopped();
            _active= false;
            return;
        }
#ifdef _WIN32
        //loginfo("started: %p tid=%08lx - %s\n", this, GetCurrentThreadId(), name());
#else
        //loginfo("started: %p - %s\n", this, name());
#endif
        try {   // catch double exceptions
        try {
        at_service_start();
        while (!_terminating && !_stopped)
        {
            //loginfo("servicing %p - %s\n", this, name());
            service();
        }
        //loginfo("ending service %s [ term=%d, stop=%d ]\n", name(), _terminating, _stopped);
        }
        catch(const char*e)
        {
            logmsg("\nservice: %p %s E:- %s\n", this, name(), e);
            handle_error();
        }
        catch(...)
        {
            logmsg("error in service %p %s\n", this, name());
            handle_error();
        }
        }
        catch(...)
        {
            logerror("NOTE: double exception - exception in process::handle_error(%p)\n", this);
        }
        // todo: should i call a 'handlestop' or 'handle_error', so the process's subclass
        // can take action?
        stopped();
        _active= false;
        // todo: can i call processstop(); here once more?
    }
private:
    volatile bool _started;
    bool _terminatingself;
    int32_t _terminating;
    volatile bool _stopped;
    volatile bool _active;
    std::condition_variable _startcond;
    std::condition_variable _stopcond;
    std::mutex _processmtx;
    // note: _thread must be -after- _processmtx
    std::thread _thread;

    void wait_for_stop()
    {
        std::unique_lock<std::mutex> lock(_processmtx);
        while (_started && _active && !_stopped)
            _stopcond.wait(lock);
    }

    void wait_for_start()
    {
        std::unique_lock<std::mutex> lock(_processmtx);
        while (!_started && !_stopped && !_terminating)
            _startcond.wait(lock);
    }
};
struct runaftertime : process {
    typedef std::function<void ()> CB;

    runaftertime(CB cb, int timeout) : _callback(cb), _timeout(timeout)
    {
        start();
    }
    virtual ~runaftertime()
    {
        stop();
    }
    virtual void processstop()
    {
        _cond.notify_one();
    }
    virtual const char*name() { return "runaftertime"; }
    virtual void service()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        while (!isterminating())
        {
            int timeleft= _timeout-_timer.msecelapsed();
            if (timeleft<0)
                break;
            _cond.wait_for(lock, std::chrono::milliseconds(timeleft));
        }
        if (!isterminating()) {
            _callback();
        }
        terminateservice();
    }

    std::mutex _mtx;
    std::condition_variable _cond;
    CB _callback;
    int _timeout;
    HiresTimer _timer;
};
#endif

