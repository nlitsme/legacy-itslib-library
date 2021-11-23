#ifndef _UTIL_RW_COMPRESSEDREADER_H__
#define _UTIL_RW_COMPRESSEDREADER_H__
#include <map>
#include <zlib.h>
class ZAllocator {
private:
    typedef std::shared_ptr<std::vector<uint8_t>> ByteVector_ptr;
    typedef std::map<voidpf,ByteVector_ptr> ptrmap_t;

    ptrmap_t _memmap;
public:
    void initstream(z_stream& _zs)
    {
        _zs.opaque= this;
        _zs.zalloc= &ZAllocator::alloc;
        _zs.zfree= &ZAllocator::free;
    }

    static voidpf alloc(voidpf opaque, uInt items, uInt size)
    {
        return static_cast<ZAllocator*>(opaque)->alloc(items, size);
    }
    static void   free(voidpf opaque, voidpf address)
    {
        return static_cast<ZAllocator*>(opaque)->free(address);
    }
    voidpf alloc(uInt items, uInt size)
    {
        ByteVector_ptr m= ByteVector_ptr(new std::vector<uint8_t>(size*items));
        _memmap.insert(ptrmap_t::value_type(&m->front(),m));
        //debug("alloc(%d,%d)->%08lx\n", items, size, &m->front());
        return &m->front();
    }
    void   free(voidpf address)
    {
        //debug("free(%08lx)\n", address);
        if (!_memmap.erase(address))
            throw "invalid address";
    }
};


class CompressedReader : public ReadWriter {
    ReadWriter_ptr _f;
    ZAllocator _za;
    z_stream _zs;

    std::vector<uint8_t> _inbuf;
    std::vector<uint8_t> _outbuf;
    size_t _outtail;
    uint64_t _curpos;
    bool _eof;

public:
    CompressedReader(ReadWriter_ptr f, bool gzipped= false)
        : _f(f), _outtail(0), _curpos(0), _eof(false)
    {
        setreadonly();

        memset(&_zs, 0, sizeof(_zs));
        _za.initstream(_zs);
        int stat= inflateInit2(&_zs, 15+(gzipped?16:0));
        if (stat!=Z_OK)
            throw "inflateinit error";
        _inbuf.resize(65536);
        _outbuf.resize(65536);
        _zs.avail_out= 65536;
        _zs.next_out= &_outbuf.front();
    }
    virtual ~CompressedReader()
    {
        inflateEnd(&_zs);
    }

    // [       |         |        ]
    // <--tail->         <-avail-->
    //
    virtual size_t read(uint8_t *p, size_t n)
    {
        //printf("read(%d)  comp: in:%d, out:%d/%d.  @%llx\n", n, _inbuf.size(), _outbuf.size(), _outtail, _curpos);
        size_t totalread= 0;
        size_t want= std::min(n, _outbuf.size()-_zs.avail_out-_outtail);
        if (want) {
            //printf("want1= %d\n", want);
            if (p) {
                memcpy(p, &_outbuf[_outtail], want);
                p+= want;
            }
            totalread+= want;
            n-= want;
            _outtail+= want;
        }
        if (n==0) {
            _curpos += totalread;
            return totalread;
        }

        // if no more inputdata -> read more
        if (_zs.avail_in==0) {
            _zs.avail_in= _f->read(&_inbuf[0], _inbuf.size());
            _zs.next_in= &_inbuf[0];

            //printf("comp: read %d bytes\n", _inbuf.size(), hexdump(&_inbuf[0], std::min(size_t(64), _inbuf.size())).c_str());
        }

        if (_zs.avail_out==0) {
            size_t remaining= _outbuf.size()-_zs.avail_out-_outtail;
            if (remaining>0) {
                memmove(&_outbuf[0], &_outbuf[_outtail], remaining);
            }
            _zs.next_out= &_outbuf.front()+remaining;
            _zs.avail_out= _outbuf.size()-remaining;
            _outtail= 0;
        }

        int stati= inflate(&_zs, Z_SYNC_FLUSH);
        if (stati!=Z_STREAM_END && stati!=Z_OK) {
            _curpos += totalread;
            throw "inflate error";
        }
        if (stati==Z_STREAM_END) {
            //printf("comp: eof\n");
            _eof= true;
        }

        size_t want2= std::min(n, _outbuf.size()-_zs.avail_out-_outtail);
        if (want2) {
            //printf("comp: want2= %d\n", want2);
            if (p) {
                memcpy(p, &_outbuf[_outtail], want2);
                p+= want2;
            }
            totalread+= want2;
            n-= want2;
            _outtail+= want2;
        }
        _curpos += totalread;

        //printf("comp -> %d bytes\n", totalread);
        return totalread;
    }
    virtual void write(const uint8_t *p, size_t n)
    {
        throw "CompressedReader cannot write";
    }
    virtual void truncate(uint64_t off)
    {
        throw "CompressedReader: truncate not implemented.";
    }

    virtual void setpos(uint64_t off)
    {
        if (off < _curpos)
            throw "CompressedReader: backwards seeking not implemented.";
        while (!eof() && off > _curpos)
            read(NULL, off-_curpos);
    }
    virtual uint64_t size()
    {
        throw "CompressedReader: size not implemented";
    }
    virtual uint64_t getpos() const
    {
        return _curpos;
    }
    virtual bool eof()
    {
        return _eof && _outtail==_outbuf.size();
    }
};

#endif
