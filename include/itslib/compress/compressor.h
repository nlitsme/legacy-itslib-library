#ifndef _COMPRESS_COMPRESSOR_H_
#define _COMPRESS_COMPRESSOR_H_
class decompressor {
public:
    virtual size_t add(const uint8_t *data, size_t size)= 0;
    virtual size_t get(uint8_t *data, size_t size)= 0;
    virtual bool eof()= 0;
};

#endif
