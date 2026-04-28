/*	$211_BSD_X44: Makefile,v 1.00 2026/06/05 19:23:47 martin Exp $ */

#include <sys/types.h>
#include <lib/libkern/libkern.h>

/*
 * enhanced_double_hashing: (Double hashing with cubic function)
 */

static void enhanced_double_hashing(uint32_t x, uint32_t hashes[], int nelems);
static int isPrime(int nelems);
static int Prime(int nelems);

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
