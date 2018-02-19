// a libc replacement (more or less) for the Microsoft Visual C compiler
// this file is public domain -- do with it whatever you want!
#ifndef __LIBC_H_INCLUDED__
#define __LIBC_H_INCLUDED__

// check if minilibc is required
#ifndef NEED_MINILIBC
    #ifndef NOLIBC
        #define NEED_MINILIBC 0
    #else
        #define NEED_MINILIBC 1
    #endif
#endif

#ifdef _MSC_VER
    #define INLINE __forceinline
    #define FASTCALL __fastcall
    #ifdef NOLIBC
        #ifdef MAIN_PROGRAM
            int _fltused=0;
        #endif
    #endif
#else
    #define INLINE inline
    #define FASTCALL __attribute__((fastcall))
    #include <stdint.h>
#endif

#ifdef _WIN32
    #ifndef WIN32
        #define WIN32
    #endif
#endif
#ifdef WIN32
    #include <windows.h>
#endif

#if !NEED_MINILIBC
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
#endif
#include <math.h>

#ifndef __int8_t_defined
    #define __int8_t_defined
    typedef unsigned char  uint8_t;
    typedef   signed char   int8_t;
    typedef unsigned short uint16_t;
    typedef   signed short  int16_t;
    typedef unsigned int   uint32_t;
    typedef   signed int    int32_t;
    #ifdef _MSC_VER
        typedef unsigned __int64 uint64_t;
        typedef   signed __int64  int64_t;
    #else
        typedef unsigned long long uint64_t;
        typedef   signed long long  int64_t;
    #endif
#endif

#ifndef NULL
    #define NULL 0
#endif

#ifndef M_PI
    #define M_PI 3.14159265358979
#endif

///////////////////////////////////////////////////////////////////////////////

#if NEED_MINILIBC 

static INLINE void libc_memset(void *dest, int value, int count) {
    if (!count) return;
    __asm {
        cld
        mov edi, dest
        mov eax, value
        mov ecx, count
        rep stosb
    }
}

static INLINE void libc_memcpy(void *dest, const void *src, int count) {
    if (!count) return;
    __asm {
        cld
        mov esi, src
        mov edi, dest
        mov ecx, count
        rep movsb
    }
}

#define libc_memmove libc_memcpy

static INLINE void* libc_malloc(int size) {
    return (void*) LocalAlloc(LMEM_FIXED, size);
}

static INLINE void* libc_calloc(int size, int nmemb) {
    return (void*) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size * nmemb);
}

static INLINE void* libc_realloc(void* old, int size) {
    int oldsize = (int) LocalSize((HLOCAL) old);
    void *mem;
    if (size <= oldsize) return old;
    mem = LocalAlloc(LMEM_FIXED, size);
    libc_memcpy(mem, old, oldsize);
    LocalFree((HLOCAL) old);
    return mem;
}

static INLINE void libc_free(void *mem) {
    LocalFree((HLOCAL) mem);
}

static INLINE double libc_frexp(double x, int *e) {
    double res = -9999.999;
    unsigned __int64 i = *(unsigned __int64*)(&x);
    if (!(i & 0x7F00000000000000UL)) {
        *e = 0;
        return x;
    }
    *e = ((i << 1) >> 53) - 1022;
    i &= 0x800FFFFFFFFFFFFFUL;
    i |= 0x3FF0000000000000UL;
    return *(double*)(&i) * 0.5;
}

static INLINE double __declspec(naked) libc_exp(double x) { __asm {
    fldl2e
    fld qword ptr [esp+4]
    fmul
    fst st(1)
    frndint
    fxch
    fsub st(0), st(1)
    f2xm1
    fld1
    fadd
    fscale
    ret
} }


static INLINE double __declspec(naked) libc_pow(double b, double e) { __asm {
    fld qword ptr [esp+12]
    fld qword ptr [esp+4]
    fyl2x
// following is a copy of libc_exp:
    fst st(1)
    frndint
    fxch
    fsub st(0), st(1)
    f2xm1
    fld1
    fadd
    fscale
    ret
} }



#else // NEED_MINILIBC == 0

#define libc_malloc  malloc
#define libc_calloc  calloc
#define libc_realloc realloc
#define libc_free    free

#define libc_memset  memset
#define libc_memcpy  memcpy
#define libc_memmove memmove

#define libc_frexp   frexp
#define libc_exp     exp
#define libc_pow     pow

#endif // NEED_MINILIBC

#endif//__LIBC_H_INCLUDED__
