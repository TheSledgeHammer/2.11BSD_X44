/*      $NetBSD: rndpool.c,v 1.20 2008/04/28 20:23:47 martin Exp $        */

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.  This code uses ideas and
 * algorithms from the Linux driver written by Ted Ts'o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <dev/misc/rnd/rnd.h>

/*
 * The random pool "taps"
 */
#define	POOL_TAP1	1638
#define	POOL_TAP2	1231
#define	POOL_TAP3	819
#define	POOL_TAP4	411

void
rndpool_init(rndpool_t *rp)
{
	rp->cursor = 0;
	rp->rotate = 1;
}

u_int32_t *
rndpool_get_pool(rndpool_t *rp)
{
	return (rp->pool);
}

u_int32_t
rndpool_get_poolsize(void)
{
	return (RND_POOLWORDS);
}

static inline void
rndpool_add_one_word(rndpool_t *rp, u_int32_t  val)
{
  	/*
	 * Shifting is implemented using a cursor and taps as offsets,
	 * added mod the size of the pool. For this reason,
	 * RND_POOLWORDS must be a power of two.
	 */
	val ^= rp->pool[(rp->cursor + POOL_TAP1) & (RND_POOLMASK)];
	val ^= rp->pool[(rp->cursor + POOL_TAP2) & (RND_POOLMASK)];
	val ^= rp->pool[(rp->cursor + POOL_TAP3) & (RND_POOLMASK)];
	val ^= rp->pool[(rp->cursor + POOL_TAP4) & (RND_POOLMASK)];
	if (rp->rotate != 0) {
		val = ((val << rp->rotate) | (val >> (32 - rp->rotate)));
	}
	rp->pool[rp->cursor++] ^= val;

	/*
	 * If we have looped around the pool, increment the rotate
	 * variable so the next value will get xored in rotated to
	 * a different position.
	 */
	if (rp->cursor == RND_POOLWORDS) {
		rp->cursor = 0;
		rp->rotate = (rp->rotate + 7) & 31;
	}
}

/*
 * Add a buffer's worth of data to the pool.
 */
void
rndpool_add_data(rndpool_t *rp, void *p, u_int32_t len)
{
	u_int32_t val;
	u_int8_t *buf;

	buf = p;

	for (; len > 3; len -= 4) {
		val = *((u_int32_t *)buf);

		rndpool_add_one_word(rp, val);
		buf += 4;
	}

	if (len != 0) {
		val = 0;
		switch (len) {
		case 3:
			val = *buf++;
		case 2:
			val = val << 8 | *buf++;
		case 1:
			val = val << 8 | *buf++;
		}

		rndpool_add_one_word(rp, val);
	}
}

/*
 * Extract some number of bytes from the random pool, decreasing the
 * estimate of randomness as each byte is extracted.
 *
 * Do this by hashing the pool and returning a part of the hash as
 * randomness.  Stir the hash back into the pool.  Note that no
 * secrets going back into the pool are given away here since parts of
 * the hash are xored together before being returned.
 *
 * Honor the request from the caller to only return good data, any data,
 * etc.  Note that we must have at least 64 bits of entropy in the pool
 * before we return anything in the high-quality modes.
 */
u_int32_t
rndpool_extract_data(rndpool_t *rp, void *p, u_int32_t len)
{
	u_int i;
	u_char digest[SHA512_DIGEST_LENGTH];
	SHA512_CTX hash;
	u_int32_t remain, count;
	u_int8_t *buf;

	buf = p;
	remain = len;

	SHA512_Init(&hash);
	SHA512_Update(&hash, (u_int8_t *)rp->pool, RND_POOLBYTES);
	SHA512_Final(digest, &hash);

	for (i = 0; i < 4; i++) {
		u_int32_t word;
		memcpy(&word, &digest[i * 16], 4);
		rndpool_add_one_word(rp, word);
	}

	count = min(remain, RND_ENTROPY_THRESHOLD);

	for (i = 0; i < count; i++) {
		buf[i] = digest[i] ^ digest[i + RND_ENTROPY_THRESHOLD];
	}

	buf += count;
	remain -= count;

	memset(&hash, 0, sizeof(hash));
	memset(digest, 0, sizeof(digest));

	return (len - remain);
}
