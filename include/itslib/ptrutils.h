#ifndef __PTRUTILS_H__

#define PTR_ADD(p,n)    (((BYTE*)(p))+(n))
#define PTR_DIFF(a,b)   (((BYTE*)(b))-((BYTE*)(a)))
#define GETBYTE(ptr)    (*(BYTE*)(ptr))
#define GETWORD(ptr)    ((GETBYTE(PTR_ADD(ptr,1))<<8)|GETBYTE(ptr))
#define GETDWORD(ptr)   ((GETWORD(PTR_ADD(ptr,2))<<16)|GETWORD(ptr))


#define __PTRUTILS_H__
#endif
