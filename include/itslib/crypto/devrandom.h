#pragma once
#include <unistd.h>
#include <fcntl.h>
struct devrandom {
    int fh;
    devrandom(bool strong= true)
    {
        fh= open(strong ? "/dev/random" : "/dev/urandom", O_RDONLY);
        if (fh==-1)
            throw "open devrandom";
    }
    ~devrandom()
    {
        close(fh);
    }
    uint8_t get()
    {
        uint8_t byte;
        if (1!=read(fh, &byte, 1))
            throw "rng error";
        return byte;
    }
    uint32_t get(uint32_t m)
    {
        uint32_t val= 0;
        if (sizeof(val)!=read(fh, &val, sizeof(val)))
            throw "rng error";
        return val % m;
    }
    void get(uint8_t *bytes, size_t n)
    {
        if (n!=read(fh, bytes,n))
            throw "rng error";
    }
    static devrandom& strong()
    {
        static devrandom dev;
        return dev;
    }
    static devrandom& quick()
    {
        static devrandom udev(false);
        return udev;
    }

};
inline uint64_t quickrandomnum()
{
    uint64_t num;
    devrandom::quick().get((uint8_t*)&num, sizeof(num));
    return num;
}

inline uint64_t strongrandomnum()
{
    uint64_t num;
    devrandom::strong().get((uint8_t*)&num, sizeof(num));
    return num;
}

