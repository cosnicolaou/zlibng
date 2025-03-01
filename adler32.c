/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-2011, 2016 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* @(#) $Id$ */

#include "zbuild.h"
#include "zutil.h"
#include "functable.h"

uint32_t adler32_c(uint32_t adler, const unsigned char *buf, size_t len);
static uint32_t adler32_combine_(uint32_t adler1, uint32_t adler2, z_off64_t len2);

#define BASE 65521U     /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf, i)  {adler += (buf)[i]; sum2 += adler;}
#define DO2(buf, i)  DO1(buf, i); DO1(buf, i+1);
#define DO4(buf, i)  DO2(buf, i); DO2(buf, i+2);
#define DO8(buf, i)  DO4(buf, i); DO4(buf, i+4);
#define DO16(buf)    DO8(buf, 0); DO8(buf, 8);

/* use NO_DIVIDE if your processor does not do division in hardware --
   try it both ways to see which is faster */
#ifdef NO_DIVIDE
/* note that this assumes BASE is 65521, where 65536 % 65521 == 15
   (thank you to John Reiser for pointing this out) */
#  define CHOP(a) \
    do { \
        uint32_t tmp = a >> 16; \
        a &= 0xffff; \
        a += (tmp << 4) - tmp; \
    } while (0)
#  define MOD28(a) \
    do { \
        CHOP(a); \
        if (a >= BASE) a -= BASE; \
    } while (0)
#  define MOD(a) \
    do { \
        CHOP(a); \
        MOD28(a); \
    } while (0)
#  define MOD63(a) \
    do { /* this assumes a is not negative */ \
        z_off64_t tmp = a >> 32; \
        a &= 0xffffffffL; \
        a += (tmp << 8) - (tmp << 5) + tmp; \
        tmp = a >> 16; \
        a &= 0xffffL; \
        a += (tmp << 4) - tmp; \
        tmp = a >> 16; \
        a &= 0xffffL; \
        a += (tmp << 4) - tmp; \
        if (a >= BASE) a -= BASE; \
    } while (0)
#else
#  define MOD(a) a %= BASE
#  define MOD28(a) a %= BASE
#  define MOD63(a) a %= BASE
#endif

/* ========================================================================= */
uint32_t adler32_c(uint32_t adler, const unsigned char *buf, size_t len) {
    uint32_t sum2;
    unsigned n;

    /* split Adler-32 into component sums */
    sum2 = (adler >> 16) & 0xffff;
    adler &= 0xffff;

    /* in case user likes doing a byte at a time, keep it fast */
    if (len == 1) {
        adler += buf[0];
        if (adler >= BASE)
            adler -= BASE;
        sum2 += adler;
        if (sum2 >= BASE)
            sum2 -= BASE;
        return adler | (sum2 << 16);
    }

    /* initial Adler-32 value (deferred check for len == 1 speed) */
    if (buf == NULL)
        return 1L;

    /* in case short lengths are provided, keep it somewhat fast */
    if (len < 16) {
        while (len) {
            --len;
            adler += *buf++;
            sum2 += adler;
        }
        if (adler >= BASE)
            adler -= BASE;
        MOD28(sum2);            /* only added so many BASE's */
        return adler | (sum2 << 16);
    }

    /* do length NMAX blocks -- requires just one modulo operation */
    while (len >= NMAX) {
        len -= NMAX;
#ifndef UNROLL_LESS
        n = NMAX / 16;          /* NMAX is divisible by 16 */
#else
        n = NMAX / 8;           /* NMAX is divisible by 8 */
#endif
        do {
#ifndef UNROLL_LESS
            DO16(buf);          /* 16 sums unrolled */
            buf += 16;
#else
            DO8(buf, 0);         /* 8 sums unrolled */
            buf += 8;
#endif
        } while (--n);
        MOD(adler);
        MOD(sum2);
    }

    /* do remaining bytes (less than NMAX, still just one modulo) */
    if (len) {                  /* avoid modulos if none remaining */
#ifndef UNROLL_LESS
        while (len >= 16) {
            len -= 16;
            DO16(buf);
            buf += 16;
#else
        while (len >= 8) {
            len -= 8;
            DO8(buf, 0);
            buf += 8;
#endif
        }
        while (len) {
            --len;
            adler += *buf++;
            sum2 += adler;
        }
        MOD(adler);
        MOD(sum2);
    }

    /* return recombined sums */
    return adler | (sum2 << 16);
}

uint32_t ZEXPORT PREFIX(adler32_z)(uint32_t adler, const unsigned char *buf, size_t len) {
    return zng_functable.adler32(adler, buf, len);
}

/* ========================================================================= */
uint32_t ZEXPORT PREFIX(adler32)(uint32_t adler, const unsigned char *buf, uint32_t len) {
    return zng_functable.adler32(adler, buf, len);
}

/* ========================================================================= */
static uint32_t adler32_combine_(uint32_t adler1, uint32_t adler2, z_off64_t len2) {
    uint32_t sum1;
    uint32_t sum2;
    unsigned rem;

    /* for negative len, return invalid adler32 as a clue for debugging */
    if (len2 < 0)
        return 0xffffffff;

    /* the derivation of this formula is left as an exercise for the reader */
    MOD63(len2);                /* assumes len2 >= 0 */
    rem = (unsigned)len2;
    sum1 = adler1 & 0xffff;
    sum2 = rem * sum1;
    MOD(sum2);
    sum1 += (adler2 & 0xffff) + BASE - 1;
    sum2 += ((adler1 >> 16) & 0xffff) + ((adler2 >> 16) & 0xffff) + BASE - rem;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum1 >= BASE) sum1 -= BASE;
    if (sum2 >= ((unsigned long)BASE << 1)) sum2 -= ((unsigned long)BASE << 1);
    if (sum2 >= BASE) sum2 -= BASE;
    return sum1 | (sum2 << 16);
}

/* ========================================================================= */
uint32_t ZEXPORT PREFIX(adler32_combine)(uint32_t adler1, uint32_t adler2, z_off_t len2) {
    return adler32_combine_(adler1, adler2, len2);
}

uint32_t ZEXPORT PREFIX(adler32_combine64)(uint32_t adler1, uint32_t adler2, z_off64_t len2) {
    return adler32_combine_(adler1, adler2, len2);
}
