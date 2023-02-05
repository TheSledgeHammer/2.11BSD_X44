/*	$NetBSD: rnd.h,v 1.21 2008/04/28 20:24:11 martin Exp $	*/

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

#ifndef _DEV_RND_H_
#define _DEV_RND_H_

#include <crypto/chacha/chacha.h>
#include <crypto/sha2/sha2.h>

/*
 * Size of entropy pool in 32-bit words.  This _MUST_ be a power of 2.  Don't
 * change this unless you really know what you are doing...
 */
#define	RND_POOLWORDS			2048
#define RND_POOLBYTES			(RND_POOLWORDS * 4)
#define RND_POOLMASK			(RND_POOLWORDS - 1)
#define	RND_POOLBITS			(RND_POOLWORDS * 32)

/*
 * Number of bytes returned per hash.  This value is used in both
 * rnd.c and rndpool.c to decide when enough entropy exists to do a
 * hash to extract it.
 */
#define	RND_ENTROPY_THRESHOLD	10

/*
 * Size of the event queue.  This _MUST_ be a power of 2.
 */
#ifndef RND_EVENTQSIZE
#define	RND_EVENTQSIZE			128
#endif

typedef struct {
	u_int32_t		poolsize;
	u_int32_t 		threshold;
	u_int32_t		maxentropy;

	u_int32_t		added;
	u_int32_t		curentropy;
	u_int32_t		removed;
	u_int32_t		discarded;
	u_int32_t		generated;
} rndpoolstat_t;

typedef struct {
	u_int32_t		cursor;					/* current add point in the pool */
	u_int32_t		rotate;					/* how many bits to rotate by */
	rndpoolstat_t	stats;					/* current statistics */
	u_int32_t		pool[RND_POOLWORDS]; 	/* random pool data */
	chacha_ctx		*chacha;
} rndpool_t;

void		rndpool_init(rndpool_t *);
u_int32_t	rndpool_get_entropy_count(rndpool_t *);
void		rndpool_get_stats(rndpool_t *, void *, int);
void		rndpool_increment_entropy_count(rndpool_t *, u_int32_t);
u_int32_t 	*rndpool_get_pool(rndpool_t *);
u_int32_t	rndpool_get_poolsize(void);
void		rndpool_add_data(rndpool_t *, void *, u_int32_t);
u_int32_t	rndpool_extract_data(rndpool_t *, void *, u_int32_t);

u_int32_t	rnd_extract_data(void *, u_int32_t);
//u_int32_t	arc4random(void);
//void		arc4random_buf(void *, size_t);

#endif /* _DEV_RND_H_ */
