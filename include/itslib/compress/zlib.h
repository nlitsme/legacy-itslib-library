#ifndef __COMPRESS_ZLIB_H__
#define __COMPRESS_ZLIB_H__

#include <zlib.h>
#include "compress/compressor.h"
#include "compress/zliballoc.h"
#include "compress/zliberr.h"

class ZlibDecompress : public decompressor {
    ZlibAllocator _a;
    z_stream _zs;
    bool _eof;
public:
    ZlibDecompress()
        : _eof(false)
    {
        memset(&_zs, 0, sizeof(_zs));
        _a.initstream(_zs);
        int stat= inflateInit(&_zs);
        if (stat!=Z_OK)
            throw zliberror(stat, "inflateInit2");
    }
    ZlibDecompress(const uint8_t *data, size_t size)
        : _eof(false)
    {
        memset(&_zs, 0, sizeof(_zs));
        _a.initstream(_zs);

        add(data, size);

        int stat= inflateInit(&_zs);
        if (stat!=Z_OK)
            throw zliberror(stat, "inflateInit");
    }
    // returns amount of compressed data actually used
    virtual size_t add(const uint8_t *data, size_t size)
    {
        if (_zs.avail_in) // don't update in-buffer when we still have one
            return 0;
        _zs.next_in= const_cast<uint8_t*>(data);
        _zs.avail_in= size;

        return size;
    }

    // returns amount of decompressed data
    virtual size_t get(uint8_t *data, size_t size)
    {
        _zs.next_out= data;
        _zs.avail_out= size;

        int stat= inflate(&_zs, Z_SYNC_FLUSH);
        if (stat==Z_STREAM_END) {
            inflateEnd(&_zs);
            _eof= true;
        }
        else if (stat!=Z_OK) {
            inflateEnd(&_zs);
            throw zliberror(stat, "inflate");
        }

        return size-_zs.avail_out;
    }
    virtual bool eof() { return _eof; }
    size_t left() const { return _zs.avail_in; }
};
class ZlibCompress : public decompressor {
    ZlibAllocator _a;
    z_stream _zs;
    bool _eof;
public:
    ZlibCompress()
        : _eof(false)
    {
        memset(&_zs, 0, sizeof(_zs));
        _a.initstream(_zs);
        int stat= deflateInit2(&_zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15, 9, Z_DEFAULT_STRATEGY );
        if (stat!=Z_OK)
            throw zliberror(stat, "deflateInit2");
    }
    // returns amount of compressed data actually used
    virtual size_t add(const uint8_t *data, size_t size)
    {
        if (_zs.avail_in) // don't update in-buffer when we still have one
            return 0;
        _zs.next_in= const_cast<uint8_t*>(data);
        _zs.avail_in= size;

        return size;
    }

    // returns amount of decompressed data
    virtual size_t get(uint8_t *data, size_t size)
    {
        _zs.next_out= data;
        _zs.avail_out= size;

        //printf("[%p/%d]>[%p/%d] -deflate- ", _zs.next_in, _zs.avail_in, _zs.next_out, _zs.avail_out);
        int stat= deflate(&_zs, _zs.next_in==NULL ? Z_FINISH : 0);
        //printf("[%p/%d]>[%p/%d]  %d\n", _zs.next_in, _zs.avail_in, _zs.next_out, _zs.avail_out, stat);
        if (stat==Z_STREAM_END) {
            deflateEnd(&_zs);
            _eof= true;
        }
        else if (stat!=Z_OK) {
            deflateEnd(&_zs);
            throw zliberror(stat, "deflate");
        }

        return size-_zs.avail_out;
    }
    virtual bool eof() { return _eof; }
    size_t left() const { return _zs.avail_in; }
};
#endif

