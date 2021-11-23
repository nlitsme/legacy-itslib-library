#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "util/HiresTimer.h"
#include "util/logmsg.h"
#include "util/singleton.h"
#include "util/stream.h"

#ifdef _WIN32
#ifdef TCPSOCKUSESWINSOCK2
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <winsock.h>
#endif

#ifndef WINSOCK_VERSION
#ifdef TCPSOCKUSESWINSOCK2
#define WINSOCK_VERSION 0x0202
#else
#define WINSOCK_VERSION 0x0101
#endif
#endif


#include "stringutils.h"
#include <stdint.h>
#include <memory>

class wsaerror {
    int _err;
    int _le;
    int _wsale;
public:
    wsaerror(int err) : _err(err), _le(GetLastError()), _wsale(WSAGetLastError()) { }
    ~wsaerror() { logerror("wsaerror - %08lx %08lx %08lx\n", _err, _le, _wsale); }
};

struct posixwrapper {
static int inet_pton(int af, const char *str, void *addr)
{
#ifndef TCPSOCKUSESWINSOCK2
    unsigned long inaddr= inet_addr(str);
    memcpy(addr, &inaddr, sizeof(inaddr));
#else
    SOCKADDR_IN sa;
    int size= sizeof(sa);
    int r= WSAStringToAddress(const_cast<TCHAR*>(ToTString(str).c_str()), af, NULL, (SOCKADDR*)&sa, &size);
    if (r)
        throw wsaerror(r);
    memcpy(addr, &sa.sin_addr, sizeof(sa.sin_addr));
#endif
    return 1;
}
static char* inet_ntop(int af, const void *addr, char*str, size_t size)
{
#ifndef TCPSOCKUSESWINSOCK2
    strncpy(str, inet_ntoa(*(const in_addr*)addr), size);
#else
    SOCKADDR_IN sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family=af;
    memcpy(&sa.sin_addr, addr, sizeof(sa.sin_addr));

    std::tstring tstr;
    tstr.resize(size);
    int r= WSAAddressToString((SOCKADDR*)&sa, sizeof(sa), NULL, &tstr[0], reinterpret_cast<DWORD*>(&size));
    if (r)
        throw wsaerror(r);
    // todo: check result
    strncpy(str, ToString(tstr.c_str()).c_str(), size);
#endif

    return str;
}
#ifndef TCPSOCKUSESWINSOCK2
#define SHUT_RDWR   2
#else
#define SHUT_RDWR  SD_BOTH
#endif
#define ENOTCONN        WSAENOTCONN
#define EINPROGRESS     WSAEINPROGRESS
#define TRYAGAIN        WSATRY_AGAIN
#define ECONNREFUSED    WSAECONNREFUSED
#define EHOSTUNREACH    WSAEHOSTUNREACH
static int close(int fd)
{
    return closesocket(fd);
}
static int read(int fd, char *buf, size_t size)
{
    return recv(fd, buf, size, 0);
}
static int readoob(int fd, char *buf, size_t size)
{
    return recv(fd, buf, size, MSG_OOB);
}
static int write(int fd, const char *buf, size_t size)
{
    return send(fd, buf, size, 0);
}
static int writeoob(int fd, const char *buf, size_t size)
{
    return send(fd, buf, size, MSG_OOB);
}
static int ioctl(int fd, int cmd, unsigned long *argp)
{
    return ::ioctlsocket(fd, cmd, argp);
}
static int recvfrom(int fd, char*buf, size_t size, int flags, struct sockaddr*from, int *slen)
{
    return ::recvfrom(fd, buf, size, flags, from, slen);
}
static int sendto(int fd, const char*buf, size_t size, int flags, const struct sockaddr*from, int slen)
{
    return ::sendto(fd, buf, size, flags, from, slen);
}

};
#ifndef TCPSOCKUSESWINSOCK2
typedef int socklen_t;
#endif

#else
#include <signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>  // ioctl, FIONREAD
#include <netinet/in.h>
#include <netinet/tcp.h>        // for TCP_NODELAY
#include <arpa/inet.h>
#include <netdb.h>

// todo: replace errno+constants with platform dependent functions
//   the current way of redefining constants easily breaks.
//   .. what we need are fns that check_tryagain()'
#include <errno.h>
#define TRYAGAIN     EAGAIN

#include <unistd.h>

struct posixwrapper {
static int inet_pton(int af, const char *str, void *addr) { return ::inet_pton(af,str,addr); }
static const char* inet_ntop(int af, const void *addr, char*str, size_t size) { return ::inet_ntop(af,addr,str,size); }
static int close(int fd) { return ::close(fd); }
static int read(int fd, char *buf, size_t size) { return ::read(fd,buf,size); }
static int readoob(int fd, char *buf, size_t size) { return ::recv(fd,buf,size,MSG_OOB); }
static int write(int fd, const char *buf, size_t size) { return ::write(fd,buf,size); }
static int writeoob(int fd, const char *buf, size_t size) { return ::send(fd,buf,size,MSG_OOB); }
static int ioctl(int fd, int cmd, unsigned long *argp) { return ::ioctl(fd, cmd, argp); }
static int recvfrom(int fd, char*buf, size_t size, int flags, struct sockaddr*from, socklen_t *slen)
{
    return ::recvfrom(fd, buf, size, flags, from, slen);
}
static int sendto(int fd, const char*buf, size_t size, int flags, const struct sockaddr*from, socklen_t slen)
{
    return ::sendto(fd, buf, size, flags, from, slen);
}


};
#endif

class initsocketlibrary : public Singleton<initsocketlibrary> {
public:
    initsocketlibrary() {
#ifdef _WIN32
        WSADATA WSAData;
        int r = WSAStartup(WINSOCK_VERSION,&WSAData);
        if (r)
            throw wsaerror(r);
#else
        signal(SIGPIPE, SIG_IGN);
#endif
    }
};


#ifndef _WIN32
#include <fcntl.h>
#endif
#include <string.h>

#include <string>
//#include "util/stringfuncs.h"
#include "stringutils.h"

#ifdef _WIN32_WCE
// for some strange reason 'clock' is not in wince, while clock_t is.
namespace std {
#define CLOCKS_PER_SEC 1000
    inline clock_t clock()
    {
        return GetTickCount();
    }
}
#endif

#include <util/exceptdef.h>
declareerror(servererror)

class posixerrordecoder {
    private:
    const char**_posixerrors;
    int _count;
    public:
    posixerrordecoder() {
static const char*posixerrors[]= {
"OK",                          /*   0  */
"EPERM",                       /*   1 Operation not permitted */
"ENOENT",                      /*   2 No such file or directory */
"ESRCH",                       /*   3 No such process */
"EINTR",                       /*   4 Interrupted system call */
"EIO",                         /*   5 Input/output error */
"ENXIO",                       /*   6 Device not configured */
"E2BIG",                       /*   7 Argument list too long */
"ENOEXEC",                     /*   8 Exec format error */
"EBADF",                       /*   9 Bad file descriptor */
"ECHILD",                      /*  10 No child processes */
"EDEADLK",                     /*  11 Resource deadlock avoided */
"ENOMEM",                      /*  12 Cannot allocate memory */
"EACCES",                      /*  13 Permission denied */
"EFAULT",                      /*  14 Bad address */
"ENOTBLK",                     /*  15 Block device required */
"EBUSY",                       /*  16 Device / Resource busy */
"EEXIST",                      /*  17 File exists */
"EXDEV",                       /*  18 Cross-device link */
"ENODEV",                      /*  19 Operation not supported by device */
"ENOTDIR",                     /*  20 Not a directory */
"EISDIR",                      /*  21 Is a directory */
"EINVAL",                      /*  22 Invalid argument */
"ENFILE",                      /*  23 Too many open files in system */
"EMFILE",                      /*  24 Too many open files */
"ENOTTY",                      /*  25 Inappropriate ioctl for device */
"ETXTBSY",                     /*  26 Text file busy */
"EFBIG",                       /*  27 File too large */
"ENOSPC",                      /*  28 No space left on device */
"ESPIPE",                      /*  29 Illegal seek */
"EROFS",                       /*  30 Read-only file system */
"EMLINK",                      /*  31 Too many links */
"EPIPE",                       /*  32 Broken pipe */
"EDOM",                        /*  33 Numerical argument out of domain */
"ERANGE",                      /*  34 Result too large */
"EAGAIN",                      /*  35 Resource temporarily unavailable */
"EINPROGRESS",                 /*  36 Operation now in progress */
"EALREADY",                    /*  37 Operation already in progress */
"ENOTSOCK",                    /*  38 Socket operation on non-socket */
"EDESTADDRREQ",                /*  39 Destination address required */
"EMSGSIZE",                    /*  40 Message too long */
"EPROTOTYPE",                  /*  41 Protocol wrong type for socket */
"ENOPROTOOPT",                 /*  42 Protocol not available */
"EPROTONOSUPPORT",             /*  43 Protocol not supported */
"ESOCKTNOSUPPORT",             /*  44 Socket type not supported */
"ENOTSUP",                     /*  45 Operation not supported */
"EPFNOSUPPORT",                /*  46 Protocol family not supported */
"EAFNOSUPPORT",                /*  47 Address family not supported by protocol family */
"EADDRINUSE",                  /*  48 Address already in use */
"EADDRNOTAVAIL",               /*  49 Can't assign requested address */
"ENETDOWN",                    /*  50 Network is down */
"ENETUNREACH",                 /*  51 Network is unreachable */
"ENETRESET",                   /*  52 Network dropped connection on reset */
"ECONNABORTED",                /*  53 Software caused connection abort */
"ECONNRESET",                  /*  54 Connection reset by peer */
"ENOBUFS",                     /*  55 No buffer space available */
"EISCONN",                     /*  56 Socket is already connected */
"ENOTCONN",                    /*  57 Socket is not connected */
"ESHUTDOWN",                   /*  58 Can't send after socket shutdown */
"ETOOMANYREFS",                /*  59 Too many references: can't splice */
"ETIMEDOUT",                   /*  60 Operation timed out */
"ECONNREFUSED",                /*  61 Connection refused */
"ELOOP",                       /*  62 Too many levels of symbolic links */
"ENAMETOOLONG",                /*  63 File name too long */
"EHOSTDOWN",                   /*  64 Host is down */
"EHOSTUNREACH",                /*  65 No route to host */
"ENOTEMPTY",                   /*  66 Directory not empty */
"EPROCLIM",                    /*  67 Too many processes */
"EUSERS",                      /*  68 Too many users */
"EDQUOT",                      /*  69 Disc quota exceeded */
"ESTALE",                      /*  70 Stale NFS file handle */
"EREMOTE",                     /*  71 Too many levels of remote in path */
"EBADRPC",                     /*  72 RPC struct is bad */
"ERPCMISMATCH",                /*  73 RPC version wrong */
"EPROGUNAVAIL",                /*  74 RPC prog. not avail */
"EPROGMISMATCH",               /*  75 Program version wrong */
"EPROCUNAVAIL",                /*  76 Bad procedure for program */
"ENOLCK",                      /*  77 No locks available */
"ENOSYS",                      /*  78 Function not implemented */
"EFTYPE",                      /*  79 Inappropriate file type or format */
"EAUTH",                       /*  80 Authentication error */
"ENEEDAUTH",                   /*  81 Need authenticator */
"EPWROFF",                     /*  82 Device power is off */
"EDEVERR",                     /*  83 Device error, e.g. paper out */
"EOVERFLOW",                   /*  84 Value too large to be stored in data type */
"EBADEXEC",                    /*  85 Bad executable */
"EBADARCH",                    /*  86 Bad CPU type in executable */
"ESHLIBVERS",                  /*  87 Shared library version mismatch */
"EBADMACHO",                   /*  88 Malformed Macho file */
"ECANCELED",                   /*  89 Operation canceled */
"EIDRM",                       /*  90 Identifier removed */
"ENOMSG",                      /*  91 No message of desired type */   
"EILSEQ",                      /*  92 Illegal byte sequence */
"ENOATTR",                     /*  93 Attribute not found */
"EBADMSG",                     /*  94 Bad message */
"EMULTIHOP",                   /*  95 Reserved */
"ENODATA",                     /*  96 No message available on STREAM */
"ENOLINK",                     /*  97 Reserved */
"ENOSR",                       /*  98 No STREAM resources */
"ENOSTR",                      /*  99 Not a STREAM */
"EPROTO",                      /* 100 Protocol error */
"ETIME",                       /* 101 STREAM ioctl timeout */
"EOPNOTSUPP",                  /* 102 Operation not supported on socket */
"ENOPOLICY",                   /* 103 No such policy registered */
};
        _posixerrors= posixerrors;
        _count= sizeof(posixerrors)/sizeof(*posixerrors);
}

const char*decode(int err)
{
    if (err>=10000)
        return winsockerror(err);
    if (err<0 || err>=_count)
        return "?";
    return _posixerrors[err];
}
const char*winsockerror(int err)
{
    switch(err)
    {
case 10004: return "WSAEINTR";
case 10009: return "WSAEBADF";
case 10013: return "WSAEACCES";
case 10014: return "WSAEFAULT";
case 10022: return "WSAEINVAL";
case 10024: return "WSAEMFILE";
case 10035: return "WSAEWOULDBLOCK";
case 10036: return "WSAEINPROGRESS";
case 10037: return "WSAEALREADY";
case 10038: return "WSAENOTSOCK";
case 10039: return "WSAEDESTADDRREQ";
case 10040: return "WSAEMSGSIZE";
case 10041: return "WSAEPROTOTYPE";
case 10042: return "WSAENOPROTOOPT";
case 10043: return "WSAEPROTONOSUPPORT";
case 10044: return "WSAESOCKTNOSUPPORT";
case 10045: return "WSAEOPNOTSUPP";
case 10046: return "WSAEPFNOSUPPORT";
case 10047: return "WSAEAFNOSUPPORT";
case 10048: return "WSAEADDRINUSE";
case 10049: return "WSAEADDRNOTAVAIL";
case 10050: return "WSAENETDOWN";
case 10051: return "WSAENETUNREACH";
case 10052: return "WSAENETRESET";
case 10053: return "WSAECONNABORTED";
case 10054: return "WSAECONNRESET";
case 10055: return "WSAENOBUFS";
case 10056: return "WSAEISCONN";
case 10057: return "WSAENOTCONN";
case 10058: return "WSAESHUTDOWN";
case 10059: return "WSAETOOMANYREFS";
case 10060: return "WSAETIMEDOUT";
case 10061: return "WSAECONNREFUSED";
case 10062: return "WSAELOOP";
case 10063: return "WSAENAMETOOLONG";
case 10064: return "WSAEHOSTDOWN";
case 10065: return "WSAEHOSTUNREACH";
case 10066: return "WSAENOTEMPTY";
case 10067: return "WSAEPROCLIM";
case 10068: return "WSAEUSERS";
case 10069: return "WSAEDQUOT";
case 10070: return "WSAESTALE";
case 10071: return "WSAEREMOTE";
case 10101: return "WSAEDISCON";
case 10102: return "WSAENOMORE";
case 10103: return "WSAECANCELLED";
case 10104: return "WSAEINVALIDPROCTABLE";
case 10105: return "WSAEINVALIDPROVIDER";
case 10106: return "WSAEPROVIDERFAILEDINIT";
case 10112: return "WSAEREFUSED";
case 10900: return "WSAEDUPLICATE_NAME";
    }
    return "?";
}
};
class socketerror {
    int _errval;
    std::string _msg;
    uint32_t _le;
    uint32_t _wsale;

public:
    socketerror(const char* fmt, ...) : 
#ifndef _WIN32_WCE
        _errval(errno)
#else
        _errval(0)
#endif
#ifdef _WIN32
        , _le(GetLastError())
        , _wsale(WSAGetLastError())
#else
        , _le(0)
        , _wsale(0)
#endif
    {
        va_list ap;
        va_start(ap, fmt);
        _msg= stringvformat(fmt, ap);
        va_end(ap);
    }
    ~socketerror() { 
        logerror("@%08lx: SOCKETERROR-: %3d/%3d/%3d[%-20s] : %s\n", HiresTimer::msecstamp(), _errval, _le, _wsale, decode(_errval), _msg.c_str());
    }
    int value() const { return _errval?_errval:_le; }
private:
    const char*decode(int errval) {
        static posixerrordecoder ps;
        return ps.decode(errval);
    }
};
class tcpaddress {
public:
    struct sockaddr_in inaddr;
    tcpaddress()
    {
        initsocketlibrary::instance();

        memset(&inaddr, 0, sizeof(inaddr));
        inaddr.sin_family = PF_INET;
        inaddr.sin_port = htons(INADDR_ANY);
        inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    // valid strings: "", "a.b.c.d", "a.b.c.d:p"
    explicit tcpaddress(const std::string& address)
    {
        memset(&inaddr, 0, sizeof(inaddr));

        initsocketlibrary::instance();

        size_t icolon= address.find(':');
        // explicitly adding icolon==0,  so a string ":1234" will fail on both win32 and unix platforms
        if (icolon!=address.npos && (icolon+1==address.size() || icolon==0))
            throw socketerror("invalid address string");

        if (icolon==address.npos)
            set(address, 0);
        else
            set(address.substr(0,icolon), strtoul(address.substr(icolon+1).c_str(),0,0));
    }
    void set(const std::string& addr, int port)
    {
        struct addrinfo hint;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;
        hint.ai_socktype = SOCK_STREAM;
        hint.ai_protocol = IPPROTO_TCP;

        struct addrinfo *ai;
        int rc= getaddrinfo(addr.c_str(), stringformat("%d", port).c_str(), &hint, &ai);
        if (rc==0) {

            for (struct addrinfo *p= ai ; p ; p=p->ai_next)
            {
                if (p->ai_family==AF_INET) {
                    inaddr.sin_family = PF_INET;
                    inaddr.sin_port = htons(port);
                    inaddr.sin_addr = ((struct sockaddr_in*)p->ai_addr)->sin_addr;
                    return;
                }
            }
            freeaddrinfo(ai);
        }
        else {
            inaddr.sin_family = PF_INET;
            inaddr.sin_port = htons(port);
            int32_t i32Res = posixwrapper::inet_pton(inaddr.sin_family, addr.c_str(), &inaddr.sin_addr);
            if(i32Res < 0)
                throw socketerror("inet_pton: invalid AF");
            else if(i32Res==0)
                throw socketerror("inet_pton: invalid ipaddress");
            return;
        }
    }
    std::string asstring() const
    {
        std::string ipaddress; ipaddress.resize(256);
        if (NULL==posixwrapper::inet_ntop(PF_INET, &inaddr.sin_addr, &ipaddress[0], ipaddress.size()))
            throw socketerror("inet_ntop");
        ipaddress.resize(strlen(ipaddress.c_str()));
        if (port())
            return stringformat("%s:%d", ipaddress.c_str(), port());
        else
            return stringformat("%s", ipaddress.c_str());
    }
    explicit tcpaddress(int port)
    {
        memset(&inaddr, 0, sizeof(inaddr));
     
        inaddr.sin_family = PF_INET;
        inaddr.sin_port = htons(port);
        inaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
 
    socklen_t size() const { return (socklen_t)sizeof(inaddr); }


    bool operator<(const tcpaddress& rhs) const
    {
        if (inaddr.sin_family<rhs.inaddr.sin_family)
            return true;
        if (inaddr.sin_family>rhs.inaddr.sin_family)
            return false;
        if (addr()<rhs.addr())
            return true;
        if (addr()>rhs.addr())
            return false;
        if (port()<rhs.port())
            return true;
        if (port()>rhs.port())
            return false;
        return false;
    }
    bool operator==(const tcpaddress& rhs) const
    {
        if (inaddr.sin_family!=rhs.inaddr.sin_family)
            return false;
        if (addr()!=rhs.addr())
            return false;
        if (port()!=rhs.port())
            return false;
        return true;
    }
    uint32_t addr() const { return ntohl(inaddr.sin_addr.s_addr); }
    uint16_t port() const { return ntohs(inaddr.sin_port); }

    bool empty() const
    {
        return inaddr.sin_family == PF_INET
            && inaddr.sin_port == htons(INADDR_ANY)
            && inaddr.sin_addr.s_addr == htonl(INADDR_ANY);
    }
};

class tcpsocket;
typedef std::shared_ptr<tcpsocket> tcpsocket_ptr;
class tcpsocket : public stream {
    int _fd;
    bool _isblocking;
    bool _remoteshutdown;
public:
    tcpsocket()
        : _isblocking(true), _remoteshutdown(false)
    {
        initsocketlibrary::instance();
        _fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(-1 == _fd)
            throw socketerror("cannot create socket");

        int on=1;
#ifdef _WIN32
#define SOCKOPT_PARAMCAST (const char*)
#else
#define SOCKOPT_PARAMCAST
#endif
        // (void*) to keep this compatible between posix and winsock
        if (-1==setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, SOCKOPT_PARAMCAST &on, sizeof(on)))
            throw socketerror("setsockopt(REUSEADDR)");
        //printf("%p new socket: %d\n", this, _fd);
    }
    explicit tcpsocket(int fd) : _fd(fd), _isblocking(true), _remoteshutdown(false)
    {
        //printf("%p fd socket: %d\n", this, _fd);
    } 
    virtual ~tcpsocket()
    {
        //printf("deleting socket(%d)\n", _fd);
        if (_fd == -1)
            return;
        try {
        //loginfo("%s ~tcp shutdown+close\n", logstamp().c_str());
        shutdown();
        close();
        }
        catch(...)
        {
            logerror("err in destructor\n");
        }
    }
    void clear()
    {
        //loginfo("cpipsock-clear(%d)\n", _fd);
        _fd= -1;
    }
    virtual void shutdown()
    {
        //loginfo("%s tcp shutdown\n", logstamp().c_str());
        if (-1==::shutdown(_fd, SHUT_RDWR)) {
#ifdef _WIN32
        if (WSAGetLastError()!=ENOTCONN)
#else
        if (errno!=ENOTCONN)
#endif
            /*nothrow*/socketerror("shutdown");
        }
    }
    void close()
    {
        //loginfo("%s tcp close\n", logstamp().c_str());
        posixwrapper::close(_fd);
        _fd= -1;
    }
    void setnodelay()
    {
        int on= 1;
        if (-1==setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, SOCKOPT_PARAMCAST &on, sizeof(on)))
            throw socketerror("setsockopt(TCP_NODELAY)");
    }
    void setwithdelay()
    {
        int on= 0;
        if (-1==setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, SOCKOPT_PARAMCAST &on, sizeof(on)))
            throw socketerror("setsockopt(TCP_NODELAY)");
    }

    void setnonblocking()
    {
        //loginfo("cpipsock-setnonblocking\n");
#ifdef _WIN32
        unsigned long nonblocking = 1;
        if (-1==ioctlsocket(_fd, FIONBIO, (unsigned long*) &nonblocking))
            throw socketerror("ioctlsocket(FIONBIO)");
#else
        int flags= fcntl(_fd, F_GETFL);
        if (flags==-1)
            throw socketerror("fcntl(GETFL)");
        if (-1==fcntl(_fd, F_SETFL, flags|O_NONBLOCK))
            throw socketerror("fcntl(SETFL)");
#endif
        _isblocking= false;
    }
    bool isblocking() { return _isblocking; }

    // use this to request the asynchronous error state
    int getsocketerror()
    {
        int val;
        socklen_t l= sizeof(int); 
        if (-1==getsockopt(_fd, SOL_SOCKET, SO_ERROR, (char*)&val, &l))
            throw socketerror("getsockopt(SO_ERROR)");

        return val;
    }

    void logsocketinfo(const char *msg)
    {
        //loginfo("%s: %s sock=%s -> peer=%s\n", logstamp().c_str(), msg, getsockname().c_str(), getpeername().c_str());
    }
    void logsocketprogress(const char *msg)
    {
        logprogress("%s sock=%s -> peer=%s\n", msg, getsockname().c_str(), getpeername().c_str());
    }

    // returns false when the operation did not yet complete
    // note: connect -> wr is signalled in select
    bool connect(const tcpaddress& addr)
    {
        ////loginfo("connect %s\n", addr.asstring().c_str());
        logsocketinfo("preconnect");
        if(-1 == ::connect(_fd, reinterpret_cast<const sockaddr*>(&addr.inaddr), addr.size()))
        {
#ifdef _WIN32
            if (WSAGetLastError()!=WSAEWOULDBLOCK)
#else
            if (errno!=EINPROGRESS)
#endif

                throw socketerror("connect(%s)", addr.asstring().c_str());
            else
                return false;
        }
        logsocketprogress("postconnect");

        return true;
    }
    void bind(const tcpaddress& addr)
    {
        loginfo("cpiptcpsock-bind %s\n", addr.asstring().c_str());
        if(-1 == ::bind(_fd, reinterpret_cast<const sockaddr*>(&addr.inaddr), addr.size()))
            throw socketerror("tcp:bind(%s)", addr.asstring().c_str());
    }
    void listen(const tcpaddress& addr)
    {
        logsocketinfo("prebind");
        //loginfo("cpipsock-listen %s\n", addr.asstring().c_str());
        bind(addr);
        logsocketinfo("postbind");
     
#define SERVERQUEUELENGTH 10
        if(-1 == ::listen(_fd, SERVERQUEUELENGTH))
            throw socketerror("listen(%s)", addr.asstring().c_str());
        logsocketprogress("postlisten");
    }
    // note: accept -> rd is signalled in select
    tcpsocket_ptr accept()
    {
        //loginfo("accept\n");

        tcpaddress client;
        socklen_t slen= sizeof(client.inaddr);
        int fd= ::accept(_fd, reinterpret_cast<sockaddr*>(&client.inaddr), &slen);
        if (fd==-1)
            throw socketerror("accept");

        //printf("cpipsock-accept(%d) -> fd=%d %s\n", fd, _fd, client.asstring().c_str());
        logsocketprogress("postaccept");
        return tcpsocket_ptr(new tcpsocket(fd));
    }
    std::string readline()
    {
        if (_remoteshutdown)
            throw socketerror("remoteshutdown");
#define QUERY_TIMEOUT 10
        time_t t0 = time(NULL);
        std::string answer;
        size_t eolpos=answer.npos;
        while (true) {
            // todo: add select here?
            answer.resize(answer.size()+256);
            int n=posixwrapper::read(_fd, &*(answer.end()-256), 256);
            //loginfo("rd: %p n=%d, answer: %s\n", &*answer.end()-256, n, answer.c_str());
            if (n==-1) {
#ifdef _WIN32
                if (WSAGetLastError()!=TRYAGAIN)
#else
                if (errno!=TRYAGAIN)
#endif

                    throw socketerror("recv");
                n=0;
            }
            else if (n==0) {
                logsocketprogress("remoteshutdown");
                _remoteshutdown= true;
                // peer has shutdown
                break;
            }
            answer.resize(answer.size()-256+n);
            if ((eolpos=answer.find('\n'))!=answer.npos) {
                break;
            }
            if ((t0 - time(NULL)) > QUERY_TIMEOUT)
                throw servererror("timeout");
        }
        if (eolpos==answer.npos) {
            throw servererror("no answer");
        }
        answer.resize(eolpos);

        return answer;
    }
    void sendline(const std::string& str)
    {
        //loginfo("wr %lu %s\n", str.size()+1, str.c_str());
        size_t ofs=0;
        while (ofs<str.size()) {
            int n=posixwrapper::write(_fd, &str[ofs], str.size()-ofs);
            if (n==-1)
                throw socketerror("write");
            ofs+=n;
        }
        char eol='\n';
        while (true)
        {
            int n=posixwrapper::write(_fd, &eol, sizeof(eol));
            if (n==-1)
                throw socketerror("write");
            if (n==1)
                break;
        }
    }
    virtual size_t write(const unsigned char* data, size_t len) 
    {
        int n=posixwrapper::write(_fd, reinterpret_cast<const char*>(data), len);
        if (n==-1)
            throw socketerror("write");
        return n;
    }
    size_t write(const std::string& data)
    {
        int n=posixwrapper::write(_fd, data.c_str(), data.size());
        if (n==-1)
            throw socketerror("write");
        return n;
    }
    bool eof() const { return _remoteshutdown; }
    virtual size_t read(unsigned char* data, size_t len) 
    {
        if (_remoteshutdown)
            throw socketerror("remoteshutdown");
        int nr=posixwrapper::read(_fd, reinterpret_cast<char*>(data), len);
        if (nr==-1) {
#ifdef _WIN32
            if (WSAGetLastError()!=TRYAGAIN)
#else
            if (errno!=TRYAGAIN)
#endif
                throw socketerror("read");
            nr= 0;
        }
        else if (nr==0) {
            logsocketprogress("remoteshutdown");
            _remoteshutdown= true;
        }
        return nr;
    }
    size_t readoob(unsigned char* data, size_t len) 
    {
        if (_remoteshutdown)
            throw socketerror("remoteshutdown");
        int nr=posixwrapper::readoob(_fd, reinterpret_cast<char*>(data), len);
        if (nr==-1) {
#ifdef _WIN32
            if (WSAGetLastError()!=TRYAGAIN)
#else
            if (errno!=TRYAGAIN)
#endif
                throw socketerror("readoob");
            nr= 0;
        }
        else if (nr==0) {
            logsocketprogress("remoteshutdown");
            _remoteshutdown= true;
        }
        return nr;
    }
    size_t read(std::string& data, size_t n)
    {
        data.resize(n);
        size_t nr= read((unsigned char*)&data[0], n);
        data.resize(nr);
        return nr;
    }
    tcpaddress getpeer()
    {
        tcpaddress  addr;
        socklen_t len= addr.size();
        if (-1==::getpeername(_fd, reinterpret_cast<sockaddr*>(&addr.inaddr), &len))
            throw socketerror("getpeername");
        return addr;
    }
    // returns local ip+portnr
    tcpaddress getsock()
    {
        tcpaddress addr;
        socklen_t len= addr.size();
        if (-1==::getsockname(_fd, reinterpret_cast<sockaddr*>(&addr.inaddr), &len)) {
            socketerror("no sockname");
        }
        return addr;
    }
    // returns remote ip+portnr
    std::string getpeername()
    {
        tcpaddress addr;
        socklen_t len= addr.size();
        if (-1==::getpeername(_fd, reinterpret_cast<sockaddr*>(&addr.inaddr), &len)) {
            socketerror("no peername");
            return "?";
        }
        return addr.asstring();
    }
    // returns local ip+portnr
    std::string getsockname()
    {
        tcpaddress addr;
        socklen_t len= addr.size();
        if (-1==::getsockname(_fd, reinterpret_cast<sockaddr*>(&addr.inaddr), &len)) {
            socketerror("no sockname");
            return "?";
        }
        return addr.asstring();
    }

    // note: accept -> rd is signalled in select
    // note: connect -> wr is signalled in select
    enum { SELECT_RD=1, SELECT_WR=2, SELECT_EV=4 };

    // specify usec==0  to 'poll'
    // specify usec==-1 for infinite wait
    int select(int64_t usec=-1)
    {
        int fd= _fd;
        if (fd==-1) throw socketerror("select_nofd");
        fd_set rdset; FD_ZERO(&rdset); FD_SET(fd, &rdset);
        fd_set wrset; FD_ZERO(&wrset); FD_SET(fd, &wrset);
        fd_set errset; FD_ZERO(&errset); FD_SET(fd, &errset);

        timeval to;
        to.tv_sec= usec/1000000;
        to.tv_usec= usec%1000000;

        int result=0;
        int n= ::select(fd+1, &rdset, &wrset, &errset, usec==-1 ? NULL : &to);
        if (n==-1) throw socketerror("select");

        // note: take a new copy of the '_fd' handle, since it may have been changed by a call to 'close' while waiting in select
        fd= _fd;
        if (fd==-1) throw socketerror("select_nofd");
        if (FD_ISSET(fd, &rdset)) result |= SELECT_RD;
        if (FD_ISSET(fd, &wrset)) result |= SELECT_WR;
        if (FD_ISSET(fd, &errset)) result |= SELECT_EV;

        return result;
    }

    // returns the nr of bytes in the socket's input buffer
    size_t bytesavailable()
    {
        unsigned long nbytes=0;
        if (-1==posixwrapper::ioctl(_fd, FIONREAD, &nbytes))
            throw socketerror("ioctl(FIONREAD)");

        return nbytes;
    }
    size_t oobbytesavailable()
    {
        unsigned long nbytes=0;
        if (-1==posixwrapper::ioctl(_fd, SIOCATMARK, &nbytes))
            throw socketerror("ioctl(SIOCATMARK)");

        return nbytes;
    }
    int fd() { return _fd; }
};


class udpsocket {
    int _fd;
public:
    udpsocket()
    {
        initsocketlibrary::instance();

        _fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(-1 == _fd)
            throw socketerror("cannot create udpsocket");
    }
    explicit udpsocket(int fd) : _fd(fd) { }
    ~udpsocket()
    {
        if (_fd == -1)
            return;
        try {
        //loginfo("%s ~udp close\n", logstamp().c_str());
        close();
        }
        catch(...)
        {
            logerror("err in destructor\n");
        }
    }
    void clear()
    {
        //loginfo("cpipudpsock-clear(%d)\n", _fd);
        _fd= -1;
    }
    void close()
    {
        //loginfo("%s udp close\n", logstamp().c_str());
        posixwrapper::close(_fd);
        _fd= -1;
    }


    void logsocketinfo(const char *msg)
    {
        //loginfo("%s: %s udpsock=%s\n", logstamp().c_str(), msg, getsockname().c_str());
    }
    void logsocketprogress(const char *msg)
    {
        logprogress("%s udpsock=%s\n", msg, getsockname().c_str());
    }

    void bind(const tcpaddress& addr)
    {
        //loginfo("cpipudpsock-bind %s\n", addr.asstring().c_str());
        if(-1 == ::bind(_fd, reinterpret_cast<const sockaddr*>(&addr.inaddr), addr.size()))
            throw socketerror("udp:bind(%s)", addr.asstring().c_str());
    }
    size_t write(const tcpaddress& dst, const unsigned char* data, size_t len) 
    {
        // note: flags can be: MSG_DONTROUTE
        int n=posixwrapper::sendto(_fd, (const char*)data, len, 0, reinterpret_cast<const sockaddr*>(&dst.inaddr), dst.size());
        if (n==-1)
            throw socketerror("sendto");
        return n;
    }
    size_t read(tcpaddress& src, unsigned char* data, size_t len) 
    {
        // note: flags can be: MSG_PEEK
        socklen_t slen= src.size();

        int nr;
        while (true) {
            nr=posixwrapper::recvfrom(_fd, (char*)data, len, 0, reinterpret_cast<sockaddr*>(&src.inaddr), &slen);
            if (nr!=-1)
                break;
#ifdef _WIN32
            // ignore icmp
            if (WSAGetLastError()!=WSAECONNRESET)
#endif
                throw socketerror("recvfrom");
        }
        return nr;
    }

    // returns local ip+portnr
    tcpaddress getsock()
    {
        tcpaddress addr;
        socklen_t len= addr.size();
        if (-1==::getsockname(_fd, reinterpret_cast<sockaddr*>(&addr.inaddr), &len)) {
            socketerror("udp:no sockname");
        }
        return addr;
    }

    // returns local ip+portnr
    std::string getsockname()
    {
        tcpaddress addr;
        socklen_t len= addr.size();
        if (-1==::getsockname(_fd, reinterpret_cast<sockaddr*>(&addr.inaddr), &len)) {
            socketerror("udp:no sockname");
            return "?";
        }
        return addr.asstring();
    }

    int fd() { return _fd; }
};
#endif
