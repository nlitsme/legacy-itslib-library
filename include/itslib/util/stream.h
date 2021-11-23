#ifndef _UTIL_STREAM_H_
#define _UTIL_STREAM_H_

class stream {
    public:
    virtual ~stream () { }
    virtual size_t write(const unsigned char* data, size_t len)=0;
    virtual size_t read(unsigned char* data, size_t len)=0;
    virtual void shutdown()=0;
};
#endif
