#ifndef _COMPRESS_ZLIBALLOC_H__
#define _COMPRESS_ZLIBALLOC_H__

#include <zlib.h>
#include <string.h>
class ZlibAllocator {
public:
    struct memblock {
        size_t size;
        uint8_t  data[1];
    };
    void initstream(z_stream& zs)
    {
        zs.opaque= this;
        zs.zalloc= &ZlibAllocator::alloc;
        zs.zfree= &ZlibAllocator::free;
    }

    static voidpf alloc(voidpf opaque, uInt items, uInt size)
    {
        return static_cast<ZlibAllocator*>(opaque)->alloc(items, size);
    }
    static void   free(voidpf opaque, voidpf address)
    {
        return static_cast<ZlibAllocator*>(opaque)->free(address);
    }
    voidpf alloc(uInt items, uInt size)
    {
        memblock *ptr= reinterpret_cast<memblock*>(new uint8_t[items*size+sizeof(size_t)]);
        ptr->size = items*size;
        //printf("NEW %p[%d] -> %p\n", ptr, ptr->size, ptr->data);
        return ptr->data;
    }
    void   free(voidpf address)
    {
        memblock *ptr= reinterpret_cast<memblock*>(static_cast<uint8_t*>(address)-sizeof(size_t));
        // erase to avoid sensitive data leaking in memory
        //printf("DEL %p[%d] -> %p == %p\n", ptr, ptr->size, ptr->data, address);
        memset(ptr->data, 0, ptr->size);
        delete reinterpret_cast<uint8_t*>(ptr);
    }
};
#endif
