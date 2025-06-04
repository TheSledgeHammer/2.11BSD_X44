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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/sysctl.h>

#include <sys/mman.h>

#define KEYSTREAM_ONLY
#include "chacha_private.h"

#ifdef __weak_alias
__weak_alias(arc4random,_arc4random)
__weak_alias(arc4random_addrandom,_arc4random_addrandom)
__weak_alias(arc4random_buf,_arc4random_buf)
__weak_alias(arc4random_stir,_arc4random_stir)
__weak_alias(arc4random_uniform,_arc4random_uniform)
#endif

#ifdef __GNUC__
#define inline __inline
#else				/* !__GNUC__ */
#define inline
#endif				/* !__GNUC__ */

#define minimum(a, b) 	((a) < (b) ? (a) : (b))

#if defined(RUN_ARC4) && (RUN_ARC4 == 0)

/* arc4 macros and functions */
struct arc4_stream {
	u_int8_t i;
	u_int8_t j;
	u_int8_t s[256];
};

static inline void 		arc4_init(struct arc4_stream *);
static inline void 		arc4_addrandom(struct arc4_stream *, u_char *, int);
static void 			arc4_stir(struct arc4_stream *);
static inline u_int8_t 		arc4_getbyte(struct arc4_stream *);
static inline u_int32_t 	arc4_getword(struct arc4_stream *);

int rs_initialized;
static struct arc4_stream rs;

#else /* !RUN_ARC4 */

/* chacha macros and functions */
#define outputsize  	64
#define inputsize   	16
#define keysize     	32
#define ivsize      	8
#define bufsize     	(inputsize * outputsize)
#define ebufsize    	(keysize + ivsize)
#define rekeybase	(1024 * 1024)

struct chacha1 {
	size_t 		have;
	size_t 		count;
};

struct chacha2 {
    	chacha_ctx 	chacha;
	u_char		buf[bufsize];
};

struct chacha_stream {
	union {
		struct chacha1 *cc1;
		struct chacha2 *cc2;
	} cs;
#define cs_cc1      cs.cc1
#define cs_have     cs_cc1->have
#define cs_count    cs_cc1->count
#define cs_cc2      cs.cc2
#define cs_chacha   cs_cc2->chacha
#define cs_buf      cs_cc2->buf
};

static inline int 		chacha_allocate(struct chacha1 **, struct chacha2 **);
static inline void 		chacha_init(struct chacha_stream *, u_char *, size_t);
static inline void		chacha_stir(struct chacha_stream *);
static inline void		chacha_rekey(struct chacha_stream *, u_char *, size_t);
static inline void		chacha_stir_if_needed(struct chacha_stream *, size_t);
static inline void 		chacha_random_buf(struct chacha_stream *, void *, size_t);
static inline void		chacha_random_u32(struct chacha_stream *, u_int32_t *);

static struct chacha_stream rs;

#endif /* RUN_ARC4 */

#if defined(RUN_ARC4) && (RUN_ARC4 == 0)
static inline void
arc4_init(as)
	struct arc4_stream *as;
{
	int n;

	for (n = 0; n < 256; n++) {
		as->s[n] = n;
	}
	as->i = 0;
	as->j = 0;

	arc4_stir(as);

	for (n = 0; n < 768 * 4; n++) {
		arc4_getbyte(as);
	}
}

static inline void
arc4_addrandom(as, dat, datlen)
	struct arc4_stream *as;
	u_char *dat;
	int     datlen;
{
	int n;
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
	struct {
		struct timeval tv;
		u_int rnd[(128 - sizeof(struct timeval)) / sizeof(u_int)];
	} rdat;
	int n;
	u_int i;
	size_t len;

	gettimeofday(&rdat.tv, NULL);

	/* use sysctl random by default, as it implements chacha20 */
	for (i = 0; i < sizeof(rdat.rnd) / sizeof(u_int); i++) {
		len = sizeof(u_int);
		if (getentropy(&rdat.rnd[i], &len) == -1) {
			break;
		}
	}

	arc4_addrandom(as, (void *)&rdat, sizeof(rdat));

	/*
	 * Throw away the first N words of output, as suggested in the
	 * paper "Weaknesses in the Key Scheduling Algorithm of RC4"
	 * by Fluher, Mantin, and Shamir.  (N = 768 in our case.)
	 */
	for (n = 0; n < 768 * 4; n++) {
		arc4_getbyte(as);
	}
}

static inline u_int8_t
arc4_getbyte(as)
	struct arc4_stream *as;
{
    uint8_t si, sj, ss;

	as->i = (as->i + 1) % 256;
	si = as->s[as->i];
	as->j = (as->j + si) % 256;
	sj = as->s[as->j];
	as->s[as->i] = sj;
	as->s[as->j] = si;
	ss = (si + sj) % 256;
	return (as->s[ss]);
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
	return (val);
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
	return (arc4_getword(&rs));
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

	do {
		r = arc4random();
	} while (__predict_false(r < minimum));

	return (r % bound);
}

#else /* !RUN_ARC4 */

static inline int
chacha_allocate(cp1, cp2)
    struct chacha1 **cp1;
    struct chacha2 **cp2;
{
	struct chacha_global {
		struct chacha1 cc1;
		struct chacha2 cc2;
	};
	struct chacha_global *cg;

	cg = (struct chacha_global*) mmap(NULL, sizeof(*cg), PROT_READ | PROT_WRITE,
	MAP_ANON | MAP_PRIVATE, -1, 0);
	if (cg == MAP_FAILED) {
		return (-1);
	}
	if (minherit((caddr_t) cg, sizeof(*cg), MAP_INHERIT_ZERO) == -1) {
		munmap((caddr_t) cg, sizeof(*cg));
		return (-1);
	}

	*cp1 = &cg->cc1;
	*cp2 = &cg->cc2;
	return (0);
}

static inline void
chacha_init(cs, buf, n)
	struct chacha_stream *cs;
	u_char *buf;
	size_t n;
{
	if (n < ebufsize) {
		return;
	}

	if (cs->cs_cc1 == NULL) {
		if (chacha_allocate(&cs->cs_cc1, &cs->cs_cc2) == -1) {
			_exit(-1);
		}
	}

	chacha_keysetup(&cs->cs_chacha, buf, keysize * ivsize);
	chacha_ivsetup(&cs->cs_chacha, buf + keysize);
}

static inline void
chacha_stir(cs)
	struct chacha_stream *cs;
{
	u_char rnd[ebufsize];
	u_int32_t rekey_fuzz = 0;

	if (getentropy(rnd, sizeof(rnd)) == -1) {
		return;
	}

	if (!cs->cs_cc1) {
		chacha_init(cs, rnd, sizeof(rnd));
	} else {
		chacha_rekey(cs, rnd, sizeof(rnd));
	}
	(void)explicit_bzero(rnd, sizeof(rnd));
	/* invalidate buf */
	cs->cs_have = 0;
	memset(cs->cs_buf, 0, sizeof(cs->cs_buf));

	/* rekey interval should not be predictable */
	chacha_encrypt_bytes(&cs->cs_chacha, (u_int8_t *)&rekey_fuzz, (u_int8_t *)&rekey_fuzz, sizeof(rekey_fuzz));
	cs->cs_count = rekeybase + (rekey_fuzz % rekeybase);
}

static inline void
chacha_rekey(cs, dat, datlen)
	struct chacha_stream *cs;
	u_char *dat;
	size_t datlen;
{
#ifndef KEYSTREAM_ONLY
	memset(cs->cs_buf, 0, sizeof(cs->cs_buf));
#endif
	
	/* fill buf with the keystream */
	chacha_encrypt_bytes(&cs->cs_chacha, cs->cs_buf, cs->cs_buf, sizeof(cs->cs_buf));
	/* mix in optional user provided data */
	if (dat) {
		size_t i, m;

		m =  minimum(datlen, ebufsize);
		for (i = 0; i < m; i++) {
			cs->cs_buf[i] ^= dat[i];
		}
	}
	/* immediately reinit for backtracking resistance */
	chacha_init(cs, cs->cs_buf, ebufsize);
	memset(cs->cs_buf, 0, ebufsize);
	cs->cs_have = sizeof(cs->cs_buf) - keysize - ivsize;
}

static inline void
chacha_stir_if_needed(cs, len)
	struct chacha_stream *cs;
	size_t len;
{
	if (!cs->cs_cc1 || cs->cs_count <= len) {
		chacha_stir(cs);
	}
	if (cs->cs_count <= len) {
		cs->cs_count = 0;
	} else {
		cs->cs_count -= len;
	}
}

static inline void
chacha_random_buf(cs, buf, n)
	struct chacha_stream *cs;
	void *buf;
	size_t n;
{
	u_char *data, *keystream;
	size_t m;

	data = (u_char *)buf;
	chacha_stir_if_needed(cs, n);
	while (n > 0) {
		if (cs->cs_have > 0) {
			m = minimum(n, cs->cs_have);
			keystream = cs->cs_buf + sizeof(cs->cs_buf) - cs->cs_have;
			memcpy(data, keystream, m);
			memset(keystream, 0, m);
			data += m;
			n -= m;
			cs->cs_have -= m;
		}
		if (cs->cs_have == 0) {
			chacha_rekey(cs, NULL, 0);
		}
	}
}

static inline void
chacha_random_u32(cs, val)
	struct chacha_stream *cs;
	u_int32_t *val;
{
	u_char *keystream;

	chacha_stir_if_needed(cs, sizeof(*val));
	if (cs->cs_have < sizeof(*val)) {
		chacha_rekey(cs, NULL, 0);
	}
	keystream = cs->cs_buf + sizeof(cs->cs_buf) - cs->cs_have;
	memcpy(val, keystream, sizeof(*val));
	memset(keystream, 0, sizeof(*val));
	cs->cs_have -= sizeof(*val);
}

u_int32_t
arc4random(void)
{
	u_int32_t val;

	chacha_random_u32(&rs, &val);
	return (val);
}

void
arc4random_stir(void)
{
	chacha_stir(&rs);
}

void
arc4random_buf(buf, len)
    void *buf;
    size_t len;
{
	chacha_random_buf(&rs, buf, len);
}

void
arc4random_addrandom(dat, datlen)
	u_char *dat;
	int     datlen;
{
	chacha_random_buf(&rs, (u_char *) dat, (int) datlen);
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

	do {
		r = arc4random();
	} while (__predict_false(r < minimum));

	return (r % bound);
}
#endif /* RUN_ARC4 */

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
