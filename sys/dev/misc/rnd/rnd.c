/*	$OpenBSD: rnd.c,v 1.225 2022/11/03 04:56:47 guenther Exp $	*/

/*
 * Copyright (c) 2011,2020 Theo de Raadt.
 * Copyright (c) 2008 Damien Miller.
 * Copyright (c) 1996, 1997, 2000-2002 Michael Shalayeff.
 * Copyright (c) 2013 Markus Friedl.
 * Copyright Theodore Ts'o, 1994, 1995, 1996, 1997, 1998, 1999.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*	$NetBSD: rnd.c,v 1.71.4.3 2011/01/07 01:35:05 riz Exp $	*/

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
#include <sys/callout.h>
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/proc.h>
#include <sys/kernel.h>

#include <crypto/chacha/chacha.h>

#include <vm/include/vm_extern.h>

#include <dev/misc/rnd/rnd.h>

#ifdef RND_DEBUG
#define	DPRINTF(l,x)      if (rnd_debug & (l)) printf x
int	rnd_debug = 0;
#else
#define	DPRINTF(l,x)
#endif

#define	RND_DEBUG_WRITE		0x0001
#define	RND_DEBUG_READ		0x0002
#define	RND_DEBUG_IOCTL		0x0004
#define	RND_DEBUG_SNOOZE	0x0008

/*
 * our select/poll queue
 */
struct selinfo rnd_selq;

/*
 * Set when there are readers blocking on data from us
 */
#define	RND_READWAITING 0x00000001
volatile u_int32_t rnd_status;

struct callout rnd_callout;

rndpool_t 	rnd_pool;
static int	rnd_ready = 0;
static int	rnd_have_entropy = 0;

static inline u_int32_t rnd_counter(void);

void	rndattach(int);

#define KEYSZ		32
#define IVSZ		8
#define BLOCKSZ		64
#define RSBUFSZ		(16*BLOCKSZ)
#define EBUFSIZE 	KEYSZ + IVSZ

static u_char 		rs_buf[RSBUFSZ];
static size_t 		rs_have;		/* valid bytes at end of rs_buf */
static size_t 		rs_count;		/* bytes till reseed */

static inline void _rs_rekey(rndpool_t *rp, u_char *dat, size_t datlen);

static inline void
_rs_init(rndpool_t *rp, u_char *buf, size_t n)
{
	KASSERT(n >= KEYSZ + IVSZ);
	rndpool_init(rp);
	chacha_keysetup(&rp->chacha, buf, KEYSZ * 8);
	chacha_ivsetup(&rp->chacha, buf + KEYSZ, NULL);
}

static void
_rs_seed(rndpool_t *rp, void *buf, size_t n)
{
	_rs_rekey(rp, buf, n);

	/* invalidate rs_buf */
	rs_have = 0;
	memset(rs_buf, 0, sizeof(rs_buf));

	rs_count = 1600000;
}

static void
_rs_stir(rndpool_t *rp)
{
	u_int32_t c;

	/*
	 * take a counter early, hoping that there's some variance in
	 * the following operations
	 */
	c = rnd_counter();
	if (c) {
		rndpool_add_data(rp, &c, sizeof(u_int32_t));
		c = rnd_counter();
		rndpool_add_data(rp, &c, sizeof(u_int32_t));
	}
	_rs_seed(rp, c, sizeof(u_int32_t));
	memset(c, 0, sizeof(u_int32_t));
}

static inline void
_rs_rekey(rndpool_t *rp, u_char *dat, size_t datlen)
{
	/* fill rs_buf with the keystream */
	chacha_encrypt_bytes(&rp->chacha, rs_buf, rs_buf, sizeof(rs_buf));
	/* mix in optional user provided data */
	if (dat) {
		size_t i, m;

		m = MIN(datlen, KEYSZ + IVSZ);
		for (i = 0; i < m; i++) {
			rs_buf[i] ^= dat[i];
		}
	}

	rndpool_add_data(rp, rs_buf, KEYSZ + IVSZ);

	/* immediately reinit for backtracking resistance */
	_rs_init(rp, rs_buf, KEYSZ + IVSZ);
	memset(rs_buf, 0, KEYSZ + IVSZ);
	rs_have = sizeof(rs_buf) - KEYSZ - IVSZ;
}

static inline void
_rs_stir_if_needed(rndpool_t *rp, size_t len)
{
	static int rs_initialized;

	if (!rs_initialized) {
		/* seeds cannot be cleaned yet, random_start() will do so */
		_rs_init(rp, rs_buf, KEYSZ + IVSZ);
		rs_count = 1024 * 1024 * 1024; /* until main() runs */
		rs_initialized = 1;
	} else if (rs_count <= len) {
		_rs_stir(rp);
	} else {
		rs_count -= len;
	}
}

static inline void
_rs_random_buf(rndpool_t *rp, void *_buf, size_t n)
{
	u_char *buf = (u_char *)_buf;
	size_t m;

	_rs_stir_if_needed(rp, n);
	while (n > 0) {
		if (rs_have > 0) {
			m = MIN(n, rs_have);
			memcpy(buf, rs_buf + sizeof(rs_buf) - rs_have, m);
			memset(rs_buf + sizeof(rs_buf) - rs_have, 0, m);
			buf += m;
			n -= m;
			rs_have -= m;
		}
		if (rs_have == 0) {
			_rs_rekey(rp, NULL, 0);
		}
	}
}

static inline void
_rs_random_u32(rndpool_t *rp, u_int32_t *val)
{
	_rs_stir_if_needed(rp, sizeof(*val));
	if (rs_have < sizeof(*val)) {
		_rs_rekey(rp, NULL, 0);
	}
	memcpy(val, rs_buf + sizeof(rs_buf) - rs_have, sizeof(*val));
	memset(rs_buf + sizeof(rs_buf) - rs_have, 0, sizeof(*val));
	rs_have -= sizeof(*val);
}

/*
 * Generate a 32-bit counter.  This should be more machine dependant,
 * using cycle counters and the like when possible.
 */
static inline u_int32_t
rnd_counter(void)
{
	struct timeval tv;

	if (rnd_ready) {
		microtime(&tv);
		return (tv.tv_sec * 1000000 + tv.tv_usec);
	}
	/* when called from rnd_init, its too early to call microtime safely */
	return (0);
}

/*
 * "Attach" the random device. This is an (almost) empty stub, since
 * pseudo-devices don't get attached until after config, after the
 * entropy sources will attach. We just use the timing of this event
 * as another potential source of initial entropy.
 */
void
rndattach(int num)
{
	u_int32_t c;

	/* Trap unwary players who don't call rnd_init() early */
	KASSERT(rnd_ready);

	/* mix in another counter */
	c = rnd_counter();
	rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t));
}

/*
 * initialize the global random pool for our use.
 * rnd_init() must be called very early on in the boot process, so
 * the pool is ready for other devices to attach as sources.
 */
void
rnd_init(void *null)
{
	if (rnd_ready) {
		return;
	}

	callout_init(&rnd_callout);
	_rs_stir(&rnd_pool);

	rnd_ready = 1;
}

u_int32_t
rnd_extract_data(void *p, u_int32_t len)
{
	int retval;

	if (!rnd_have_entropy) {
		/* Try once again to put something in the pool */
		_rs_stir(&rnd_pool);
	}
	retval = rndpool_extract_data(&rnd_pool, p, len);
	return (retval);
}

int
random_open(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (ENXIO);
}

int
random_close(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return (ENXIO);
}

int
random_read(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	u_int8_t *bf;
	u_int32_t entcnt, mode, n, nread;
	int ret;

	if (uio->uio_resid == 0) {
		return (0);
	}

	ret = 0;
	bf = malloc(RND_POOLWORDS, M_TEMP, M_WAITOK);

	while (uio->uio_resid > 0) {
		n = min(RND_POOLWORDS, uio->uio_resid);

		for (;;) {
			/*
			 * Data is not available.
			 */
			if (flag & IO_NDELAY) {
				ret = EWOULDBLOCK;
				goto out;
			}

			rnd_status |= RND_READWAITING;
			ret = tsleep(&rnd_selq, PRIBIO|PCATCH, "rndread", 0);

			if (ret) {
				goto out;
			}
		}
		nread = rnd_extract_data(bf, n);

		/*
		 * Copy (possibly partial) data to the user.
		 * If an error occurs, or this is a partial
		 * read, bail out.
		 */
		ret = uiomove((void *)bf, nread, uio);
		if (ret != 0 || nread != n) {
			goto out;
		}
	}

out:
	free(bf, M_TEMP);
	return (ret);
}

int
random_write(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	u_int8_t *bf;
	int n, ret;

	if (uio->uio_resid == 0) {
		return (0);
	}

	ret = 0;

	bf = malloc(RND_POOLWORDS, M_TEMP, M_WAITOK);

	while (uio->uio_resid > 0) {
		n = min(RND_POOLWORDS, uio->uio_resid);

		ret = uiomove((void *)bf, n, uio);
		if (ret != 0) {
			break;
		}

		/*
		 * Mix in the bytes.
		 */
		rndpool_add_data(&rnd_pool, bf, n, 0);
	}

	free(bf, M_TEMP);
	return (ret);
}

int
random_ioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	switch (cmd) {
	case FIOASYNC:
		/* No async flag in softc so this is a no-op. */
		break;
	case FIONBIO:
		/* Handled in the upper FS layer. */
		break;
	default:
		return ENOTTY;
	}
	return (0);
}
