/*
 * This is free and unencumbered software released into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 * Original Works from:
 * Skeeto: https://github.com/skeeto/hash-prospector
 *
 * hash_prospector.c: Integter Hash Functions
 */

#include <sys/types.h>
#include <lib/libkern/libkern.h>

/* prospector32 */
uint32_t
prospector32(uint32_t x)
{
    x ^= x >> 15;
    x *= UINT32_C(0x2c1b3c6d);
    x ^= x >> 12;
    x *= UINT32_C(0x297a2d39);
    x ^= x >> 15;
    return (x);
}

/*
 * lowbias32
 * exact bias: 0.10734781817103507
 */
uint32_t
lowbias32(uint32_t x)
{
    x ^= x >> 16;
    x *= UINT32_C(0x21f0aaad);
    x ^= x >> 15;
    x *= UINT32_C(0xf35a2d97);
    x ^= x >> 15;
    return (x);
}

/* lowbias32 inverse */
uint32_t
lowbias32_r(uint32_t x)
{
    x ^= x >> 15;
    x *= UINT32_C(0x37132227);
    x ^= x >> 15 ^ x >> 30;
    x *= UINT32_C(0x333c4925);
    x ^= x >> 16;
    return (x);
}

/*
 * triple32
 * exact bias: 0.020888578919738908
 */
uint32_t
triple32(uint32_t x)
{
    x ^= x >> 17;
    x *= UINT32_C(0xed5ad4bb);
    x ^= x >> 11;
    x *= UINT32_C(0xac4c1b51);
    x ^= x >> 15;
    x *= UINT32_C(0x31848bab);
    x ^= x >> 14;
    return (x);
}

/* triple32 inverse */
uint32_t
triple32_r(uint32_t x)
{
    x ^= x >> 14 ^ x >> 28;
    x *= UINT32_C(0x32b21703);
    x ^= x >> 15 ^ x >> 30;
    x *= UINT32_C(0x469e0db1);
    x ^= x >> 11 ^ x >> 22;
    x *= UINT32_C(0x79a85073);
    x ^= x >> 17;
    return (x);
}

/*
 * enhanced_double_hashing: (Double hashing with cubic function)
 */

static void enhanced_double_hashing(uint32_t x, uint32_t hashes[], int nelems);
static int isPrime(int nelems);
static int Prime(int nelems);

static __inline void
enhanced_double_hashing(uint32_t x, uint32_t hashes[], int nelems)
{
    uint32_t a = lowbias32(x) % nelems;
    uint32_t b = prospector32(x) % nelems;
    int i = 0;

    if (nelems <= 1) {
        //printf("enhanced_double_hashing: nelems (%d) must be greater than 1\n", nelems);
        return;
    }
    hashes[i] = a;
    for (i = 1; i < nelems; i++) {
        a += b; // Add quadratic difference to get cubic
        b += i; // Add linear difference to get quadratic
        // i++ adds constant difference to get linear
        hashes[i] = a;
    }
}

static __inline int
isPrime(int nelems)
{
    int cnt = 0;

    if (nelems <= 1) {
        return (-1);
    }
    for (int i = 1; i <= nelems; i++) {
        if (nelems % i == 0) {
            cnt++;
        }
    }
    if (cnt > 2) {
        //printf("is not a prime: %d\n", cnt);
        return (-1);
    }
    //printf("is a prime: %d\n", cnt);
    return (0);
}

static __inline int
Prime(int nelems)
{
    int pval = -1;
    int rval = -1;

    for (int i = 0; i < nelems; i++) {
        pval = isPrime(i);
        if (pval == 0) {
            rval = i;
        }
    }
    return (rval);
}

uint32_t
enhanced_double_hash(uint32_t x, int nelems)
{
    uint32_t hash[nelems];
    uint32_t y = 0;
    int p = Prime(nelems);

    enhanced_double_hashing(x, hash, nelems);
    if (p > nelems) {
        p = nelems;
    }
    y = hash[p];
    return (y);
}
