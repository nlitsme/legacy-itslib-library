#ifndef __UTIL_ATOMIC_OPS_H__
#define __UTIL_ATOMIC_OPS_H__
#include <stdint.h>
#ifdef _WIN32

#include <windows.h>

// see http://msdn.microsoft.com/en-us/library/aa909196.aspx
//
// --- todo: find out why int32_t*  and LPLONG  are considered 'unrelated' by MSVC ( error C2664 )
inline int32_t atomic_read(const int32_t*p)
{
    return InterlockedExchangeAdd((LPLONG)const_cast<int32_t*>(p), 0);
}
inline int32_t atomic_readwrite(int32_t*p, int32_t newval)
{
    return InterlockedExchange((LPLONG)p, newval);
}
inline int32_t atomic_readwrite_if_equal(int32_t*p, int32_t cmp, int32_t newval)
{
    return InterlockedCompareExchange((LPLONG)p, newval, cmp);
}
inline int32_t atomic_increment(int32_t *p)
{
    return InterlockedIncrement((LPLONG)p);
}
inline int32_t atomic_decrement(int32_t *p)
{
    return InterlockedDecrement((LPLONG)p);
}
inline int32_t atomic_add(int32_t *p, int32_t v)
{
    return InterlockedExchangeAdd((LPLONG)p, v) + v;
}
#endif
#ifdef __GNUC__
// see http://gcc.gnu.org/onlinedocs/gcc/Atomic-Builtins.html
//
// see /usr/include/c++/4.2.1/ext/atomicity.h
// /usr/include/c++/4.0.0/bits/atomicity.h
//    - which defines __exchange_and_add,__atomic_add  in the '__gnu_cxx' namespace
//    using __sync_fetch_and_add

template<typename T>
inline T atomic_read(T*p)
{
    return __sync_fetch_and_add(p, 0);
}
template<typename T1, typename T2>
inline T1 atomic_readwrite(T1*p, T2 newval)
{
    return __sync_lock_test_and_set(p, newval);
}
template<typename T1, typename T2, typename T3>
inline T1 atomic_readwrite_if_equal(T1*p, T2 cmp, T3 newval)
{
    return __sync_val_compare_and_swap(p, cmp, newval);
}
template<typename T>
inline T atomic_increment(T *p)
{
    return __sync_add_and_fetch(p, 1);
}
template<typename T>
inline T atomic_decrement(T *p)
{
    return __sync_sub_and_fetch(p, 1);
}
template<typename T1, typename T2>
inline T1 atomic_add(T1 *p, T2 v)
{
    return __sync_add_and_fetch(p, v);
}

#endif

#endif
