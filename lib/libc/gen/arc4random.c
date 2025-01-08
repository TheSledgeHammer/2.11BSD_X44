/*	$NetBSD: arc4random.c,v 1.5 2002/11/11 01:13:07 thorpej Exp $	*/
/*	$OpenBSD: arc4random.c,v 1.6 2001/06/05 05:05:38 pvalchev Exp $	*/

/*
 * Arc4 random number generator for OpenBSD.
 * Copyright 1996 David Mazieres <dm@lcs.mit.edu>.
 *
 * Modification and redistribution in source and binary forms is
 * permitted provided that due credit is given to the author and the
 * OpenBSD project by leaving this copyright notice intact.
 */

/*
 * This code is derived from section 17.1 of Applied Cryptography,
 * second edition, which describes a stream cipher allegedly
 * compatible with RSA Labs "RC4" cipher (the actual description of
 * which is a trade secret).  The same algorithm is used as a stream
 * cipher called "arcfour" in Tatu Ylonen's ssh package.
 *
 * Here the stream cipher has been modified always to include the time
 * when initializing the state.  That makes it impossible to
 * regenerate the same random sequence twice, so this can't be used
 * for encryption, but will generate good random numbers.
 *
 * RC4 is a registered trademark of RSA Laboratories.
 */

#include "namespace.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#ifdef __GNUC__
#define inline __inline
#else				/* !__GNUC__ */
#define inline
#endif				/* !__GNUC__ */

struct arc4_stream {
	u_int8_t i;
	u_int8_t j;
	u_int8_t s[256];
};

int     rs_initialized;
static struct arc4_stream rs;

static inline void arc4_init(struct arc4_stream *);
static inline void arc4_addrandom(struct arc4_stream *, u_char *, int);
static void arc4_stir(struct arc4_stream *);
static inline u_int8_t arc4_getbyte(struct arc4_stream *);
static inline u_int32_t arc4_getword(struct arc4_stream *);

static inline void
arc4_init(as)
	struct arc4_stream *as;
{
	int     n;

	for (n = 0; n < 256; n++)
		as->s[n] = n;
	as->i = 0;
	as->j = 0;
}

static inline void
arc4_addrandom(as, dat, datlen)
	struct arc4_stream *as;
	u_char *dat;
	int     datlen;
{
	int     n;
	u_int8_t si;

	as->i--;
	for (n = 0; n < 256; n++) {
		as->i = (as->i + 1);
		si = as->s[as->i];
		as->j = (as->j + si + dat[n % datlen]);
		as->s[as->i] = as->s[as->j];
		as->s[as->j] = si;
	}
	as->j = as->i;
}

static void
arc4_stir(as)
	struct arc4_stream *as;
{
	int     fd;
	struct {
		struct timeval tv;
		u_int rnd[(128 - sizeof(struct timeval)) / sizeof(u_int)];
	}       rdat;
	int	n;

	gettimeofday(&rdat.tv, NULL);
	fd = open("/dev/urandom", O_RDONLY);
	if (fd != -1) {
		read(fd, rdat.rnd, sizeof(rdat.rnd));
		close(fd);
	}
#ifdef KERN_URND
	else {
		int mib[2];
		u_int i;
		size_t len;

		/* Device could not be opened, we might be chrooted, take
		 * randomness from sysctl. */

		mib[0] = CTL_KERN;
		mib[1] = KERN_URND;

		for (i = 0; i < sizeof(rdat.rnd) / sizeof(u_int); i++) {
			len = sizeof(u_int);
			if (sysctl(mib, 2, &rdat.rnd[i], &len, NULL, 0) == -1)
				break;
		}
	}
#endif
	/* fd < 0 or failed sysctl ?  Ah, what the heck. We'll just take
	 * whatever was on the stack... */

	arc4_addrandom(as, (void *) &rdat, sizeof(rdat));

	/*
	 * Throw away the first N words of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4"
	 * by Fluher, Mantin, and Shamir.  (N = 256 in our case.)
	 */
	for (n = 0; n < 256 * 4; n++)
		arc4_getbyte(as);
}

static inline u_int8_t
arc4_getbyte(as)
	struct arc4_stream *as;
{
	u_int8_t si, sj;

	as->i = (as->i + 1);
	si = as->s[as->i];
	as->j = (as->j + si);
	sj = as->s[as->j];
	as->s[as->i] = sj;
	as->s[as->j] = si;
	return (as->s[(si + sj) & 0xff]);
}

static inline u_int32_t
arc4_getword(as)
	struct arc4_stream *as;
{
	u_int32_t val;
	val = arc4_getbyte(as) << 24;
	val |= arc4_getbyte(as) << 16;
	val |= arc4_getbyte(as) << 8;
	val |= arc4_getbyte(as);
	return val;
}

void
arc4random_stir(void)
{
	if (!rs_initialized) {
		arc4_init(&rs);
		rs_initialized = 1;
	}
	arc4_stir(&rs);
}

void
arc4random_addrandom(dat, datlen)
	u_char *dat;
	int     datlen;
{
	if (!rs_initialized) {
		arc4random_stir();
	}
	arc4_addrandom(&rs, dat, datlen);
}

u_int32_t
arc4random(void)
{
	if (!rs_initialized) {
		arc4random_stir();
	}
	return arc4_getword(&rs);
}

void
arc4random_buf(buf, len)
    void *buf;
    size_t len;
{
    arc4random_addrandom((u_char *)buf, (int)len);
}

u_int32_t
arc4random_uniform(bound)
    	u_int32_t bound;
{
    	u_int32_t minimum, r;
	
	/*
	 * We want a uniform random choice in [0, n), and arc4random()
	 * makes a uniform random choice in [0, 2^32).  If we reduce
	 * that modulo n, values in [0, 2^32 mod n) will be represented
	 * slightly more than values in [2^32 mod n, n).  Instead we
	 * choose only from [2^32 mod n, 2^32) by rejecting samples in
	 * [0, 2^32 mod n), to avoid counting the extra representative
	 * of [0, 2^32 mod n).  To compute 2^32 mod n, note that
	 *
	 *	2^32 mod n = 2^32 mod n - 0
	 *	  = 2^32 mod n - n mod n
	 *	  = (2^32 - n) mod n,
	 *
	 * the last of which is what we compute in 32-bit arithmetic.
	 */
    	minimum = (-bound % bound);

    	if (!rs_initialized) {
        	arc4random_stir();
    	}
    	do {
		r = arc4_getword(&rs);
        	//arc4_addrandom(&rs, (u_char *)&r, sizeof(r));
    	} while (__predict_false(r < minimum));

    	return (r % bound);
}

#ifdef notyet
/* chacha */
#include <assert.h>
#include <sys/crypto/chacha/chacha.h>

#define outputsize  64
#define inputsize   16
#define keysize     32
#define ivsize      8
#define bufsize     (outputsize * outputsize)
#define ebufsize    (keysize * ivsize)

static void chacha_init(chacha_ctx *, u_char *, size_t);

static void
chacha_init(cc, buf, n)
	chacha_ctx *cc;
	u_char *buf;
	size_t n;
{
	_DIAGASSERT(n >= ebufsize);
	chacha_keysetup(cc, buf, ebufsize);
    	chacha_ivsetup(cc, buf + keysize, NULL);
}

static void
chacha_buf(cc, buf, n)
	chacha_ctx *cc;
	void *buf;
	size_t n;
{
	u_int8_t output[bufsize];
	
	chacha_encrypt_bytes(cc, output, output, sizeof(output));
	
	(void)memcpy(cc, output, sizeof(cc));
	(void)memcpy(buf, output + sizeof(cc), n);
	(void)explicit_memset(output, 0, sizeof(output));
}

#endif

#if 0
/*-------- Test code for i386 --------*/
#include <stdio.h>
#include <machine/pctr.h>
int
main(int argc, char **argv)
{
	const int iter = 1000000;
	int     i;
	pctrval v;

	v = rdtsc();
	for (i = 0; i < iter; i++)
		arc4random();
	v = rdtsc() - v;
	v /= iter;

	printf("%qd cycles\n", v);
}
#endif
