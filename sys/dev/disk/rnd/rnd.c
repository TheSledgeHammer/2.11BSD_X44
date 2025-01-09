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
#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/sysctl.h>

#include <vm/include/vm_extern.h>

#define KEYSTREAM_ONLY
#include <crypto/chacha/chacha.h>
#include <crypto/sha2/sha2.h>

#include <dev/disk/rnd/rnd.h>

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

static struct lock_object rnd_lock;
rndpool_t 	rnd_pool;
static int	rnd_ready = 0;
static int	rnd_have_entropy = 0;

static inline u_int32_t rnd_counter(void);

/* random keystream by ChaCha */
#define	CHACHA20_KEY_SIZE		32
#define	CHACHA20_IV_SIZE		8
#define	CHACHA20_BLOCK_SIZE		64
#define	CHACHA20_BUF_SIZE		(16 * CHACHA20_BLOCK_SIZE)
#define CHACHA20_EBUF_SIZE		(CHACHA20_KEY_SIZE + CHACHA20_IV_SIZE)

static chacha_ctx 	rs;				/* chacha context for random keystream */
static u_char 		rs_buf[CHACHA20_BUF_SIZE];
static size_t 		rs_have;		/* valid bytes at end of rs_buf */
static size_t 		rs_count;		/* bytes till reseed */

static inline void	_chacha20_init(u_char *, size_t);
static void			_chacha20_stir(int);
static inline void 	_chacha20_rekey(u_char *, size_t);
static inline void	_chacha20_stir_if_needed(size_t);

dev_type_open(rndopen);
dev_type_read(rndread);
dev_type_write(rndwrite);
dev_type_ioctl(rndioctl);
dev_type_poll(rndpoll);
dev_type_kqfilter(rndkqfilter);

const struct cdevsw rnd_cdevsw = {
		.d_open = rndopen,
		.d_close = nullclose,
		.d_read = rndread,
		.d_write = rndwrite,
		.d_ioctl = rndioctl,
		.d_stop = nullstop,
		.d_tty = notty,
		.d_poll = rndpoll,
		.d_mmap = nommap,
		.d_kqfilter = rndkqfilter,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

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
rndattach(num)
	int num;
{
	u_int32_t c;

	/* Trap unwary players who don't call rnd_init() early */
	KASSERT(rnd_ready);

	/* mix in another counter */
	c = rnd_counter();
	rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
}

/*
 * initialize the global random pool for our use.
 * rnd_init() must be called very early on in the boot process, so
 * the pool is ready for other devices to attach as sources.
 */
void
rnd_init(void)
{
	u_int32_t c;

	if (rnd_ready) {
		return;
	}
	
	simple_lock_init(&rnd_lock, "rnd_lock");

	/*
	 * take a counter early, hoping that there's some variance in
	 * the following operations
	 */
	c = rnd_counter();

	rndpool_init(&rnd_pool);

	/* Mix *something*, *anything* into the pool to help it get started.
	 * However, it's not safe for rnd_counter() to call microtime() yet,
	 * so on some platforms we might just end up with zeros anyway.
	 * XXX more things to add would be nice.
	 */
	if (c) {
		rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
		c = rnd_counter();
		rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
	}
	
	_chacha20_init(rs_buf, CHACHA20_EBUF_SIZE);

	rnd_ready = 1;


#ifdef RND_VERBOSE
	printf("rnd: initialised (%u)%s", RND_POOLBITS, c ? " with counter\n" : "\n");
#endif
}

int
rndopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	if (rnd_ready == 0) {
		return (ENXIO);
	}
	if (minor(dev) == RND_DEV_URANDOM) {
		return (0);
	}

	/*
	 * If this is the strong random device and we have never collected
	 * entropy (or have not yet) don't allow it to be opened.  This will
	 * prevent waiting forever for something that just will not appear.
	 */
	if (minor(dev) == RND_DEV_RANDOM) {
		if (rnd_have_entropy == 0) {
			return (ENXIO);
		} else {
			return (0);
		}
	}

	return (ENXIO);
}

int
rndread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	u_int8_t *buf;
	u_int32_t entcnt, n, nread;
	int ret, s;

	DPRINTF(RND_DEBUG_READ,
			("Random:  Read of %d requested, flags 0x%08x\n",
					uio->uio_resid, ioflag));

	if (uio->uio_resid == 0) {
		return (0);
	}

	ret = 0;
	buf = malloc(RND_POOLWORDS, M_TEMP, M_WAITOK);

	while (uio->uio_resid > 0) {
		n = min(RND_POOLWORDS, uio->uio_resid);

		/*
		 * Make certain there is data available.  If there
		 * is, do the I/O even if it is partial.  If not,
		 * sleep unless the user has requested non-blocking
		 * I/O.
		 */
		for (;;) {

			/*
			 * How much entropy do we have?  If it is enough for
			 * one hash, we can read.
			 */
			s = splsoftclock();
			entcnt = rndpool_get_entropy_count(&rnd_pool);
			splx(s);
			if (entcnt >= RND_ENTROPY_THRESHOLD * 8) {
				break;
			}

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
		nread = rnd_extract_data(buf, n);

		/*
		 * Copy (possibly partial) data to the user.
		 * If an error occurs, or this is a partial
		 * read, bail out.
		 */
		ret = uiomove((void *)buf, nread, uio);
		if (ret != 0 || nread != n) {
			goto out;
		}
	}

out:
	free(buf, M_TEMP);
	return (ret);
}

int
rndwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	u_int8_t *buf;
	int n, ret, s;

	DPRINTF(RND_DEBUG_WRITE,
			("Random: Write of %d requested\n", uio->uio_resid));

	if (uio->uio_resid == 0) {
		return (0);
	}

	ret = 0;

	buf = malloc(RND_POOLWORDS, M_TEMP, M_WAITOK);

	while (uio->uio_resid > 0) {
		n = min(RND_POOLWORDS, uio->uio_resid);

		ret = uiomove((void *)buf, n, uio);
		if (ret != 0) {
			break;
		}

		/*
		 * Mix in the bytes.
		 */
		s = splsoftclock();
		rndpool_add_data(&rnd_pool, buf, n, 0);
		splx(s);
	}

	free(buf, M_TEMP);
	return (ret);
}

int
rndioctl(dev, cmd, addr, flag, p)
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

int
rndpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	u_int32_t entcnt;
	int revents, s;

	/*
	 * We are always writable.
	 */
	revents = events & (POLLOUT | POLLWRNORM);

	/*
	 * Save some work if not checking for reads.
	 */
	if ((events & (POLLIN | POLLRDNORM)) == 0) {
		return (revents);
	}

	/*
	 * If the minor device is not /dev/random, we are always readable.
	 */
	if (minor(dev) != RND_DEV_RANDOM) {
		revents |= events & (POLLIN | POLLRDNORM);
		return (revents);
	}

	/*
	 * Make certain we have enough entropy to be readable.
	 */
	s = splsoftclock();
	entcnt = rndpool_get_entropy_count(&rnd_pool);
	splx(s);

	if (entcnt >= RND_ENTROPY_THRESHOLD * 8) {
		revents |= events & (POLLIN | POLLRDNORM);
	} else {
		selrecord(p, &rnd_selq);
	}

	return (revents);
}

static void
filt_rnddetach(kn)
	struct knote *kn;
{
	int s;

	s = splsoftclock();
	SIMPLEQ_REMOVE(&rnd_selq.si_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_rndread(kn, hint)
	struct knote *kn;
	long hint;
{
	uint32_t entcnt;

	entcnt = rndpool_get_entropy_count(&rnd_pool);
	if (entcnt >= RND_ENTROPY_THRESHOLD * 8) {
		kn->kn_data = RND_POOLWORDS;
		return (1);
	}
	return (0);
}

static int
filt_rndwrite(kn, hint)
	struct knote *kn;
	long hint;
{
	kn->kn_data = RND_POOLBYTES;
	return (1);
}

static const struct filterops rnd_seltrue_filtops =
	{ 1, NULL, filt_rnddetach, filt_seltrue };

static const struct filterops rndwrite_filtops =
	{ 1, NULL, filt_rnddetach, filt_rndwrite };

static const struct filterops rndread_filtops =
	{ 1, NULL, filt_rnddetach, filt_rndread };

int
rndkqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &rnd_selq.si_klist;
		if (minor(dev) == RND_DEV_URANDOM) {
			kn->kn_fop = &rnd_seltrue_filtops;
		} else {
			kn->kn_fop = &rndread_filtops;
		}
		break;

	case EVFILT_WRITE:
		klist = &rnd_selq.si_klist;
		if (minor(dev) == RND_DEV_URANDOM) {
			kn->kn_fop = &rnd_seltrue_filtops;
		} else {
			kn->kn_fop = &rndwrite_filtops;
		}
		break;

	default:
		return (1);
	}

	kn->kn_hook = NULL;

	s = splsoftclock();
	SIMPLEQ_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

u_int32_t
rnd_extract_data(p, len)
	void *p;
	u_int32_t len;
{
	int retval, s;
	u_int32_t c;

	s = splsoftclock();
	if (!rnd_have_entropy) {
#ifdef RND_VERBOSE
		printf("rnd: WARNING! initial entropy low (%u).\n",
		       rndpool_get_entropy_count(&rnd_pool));
#endif

		/* Try once again to put something in the pool */
		c = rnd_counter();
		rndpool_add_data(&rnd_pool, &c, sizeof(u_int32_t), 1);
		_chacha20_stir_if_needed(len);
	}
	retval = rndpool_extract_data(&rnd_pool, p, len);
	splx(s);
	return (retval);
}

/* random keystream by ChaCha */

static inline void
_chacha20_init(buf, n)
	u_char *buf;
	size_t n;
{
	KASSERT(n >= CHACHA20_EBUF_SIZE);
	chacha_keysetup(&rs, buf, CHACHA20_KEY_SIZE * CHACHA20_IV_SIZE);
	chacha_ivsetup(&rs, buf + CHACHA20_KEY_SIZE, NULL);
}

static void
_chacha20_seed(buf, n)
	u_char *buf;
	size_t n;
{
	_chacha20_rekey(buf, n);

	/* invalidate rs_buf */
	rs_have = 0;
	memset(rs_buf, 0, sizeof(rs_buf));

	rs_count = 1600000;
}

static void
_chacha20_stir(do_lock)
	int do_lock;
{
	u_int8_t buf[CHACHA20_EBUF_SIZE], *p;
	u_int32_t c;
	int i;

	 c = rndpool_extract_data(&rnd_pool, buf, sizeof(buf));
	 for (p = (u_int8_t *)&c, i = 0; i < sizeof(c); i++) {
		 buf[i] ^= p[i];
	 }

	 if (do_lock) {
		 simple_lock(&rnd_lock);
	 }
	 _chacha20_seed(buf, sizeof(buf));
	 if (do_lock) {
		 simple_unlock(&rnd_lock);
	 }
	 memset(buf, 0, sizeof(buf));
}

static inline void
_chacha20_stir_if_needed(len)
	size_t len;
{
	static int rs_initialized;

	if (!rs_initialized) {
		/* seeds cannot be cleaned yet, random_start() will do so */
		_chacha20_init(rs_buf, CHACHA20_EBUF_SIZE);
		rs_count = 1024 * 1024 * 1024; /* until main() runs */
		rs_initialized = 1;
	} else if (rs_count <= len) {
		_chacha20_stir(0);
	} else {
		rs_count -= len;
	}
}

static inline void
_chacha20_rekey(dat, datlen)
	u_char *dat;
	size_t datlen;
{
#ifndef KEYSTREAM_ONLY
	memset(rs_buf, 0, sizeof(rs_buf));
#endif

	/* fill rs_buf with the keystream */
	chacha_encrypt_bytes(&rs, rs_buf, rs_buf, sizeof(rs_buf));
	/* mix in optional user provided data */
	if (dat) {
		size_t i, m;

		m = MIN(datlen, CHACHA20_EBUF_SIZE);
		for (i = 0; i < m; i++) {
			rs_buf[i] ^= dat[i];
		}
	}

	/* immediately reinit for backtracking resistance */
	_chacha20_init(rs_buf, CHACHA20_EBUF_SIZE);
	memset(rs_buf, 0, CHACHA20_EBUF_SIZE);
	rs_have = sizeof(rs_buf) - CHACHA20_KEY_SIZE - CHACHA20_IV_SIZE;
}

static inline void
_chacha20_random_buf(buf, n)
	void *buf;
	size_t n;
{
	u_char *data;
	size_t m;

	data = (u_char *)buf;
	_chacha20_stir_if_needed(n);
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
			_chacha20_rekey(NULL, 0);
		}
	}
}

static inline void
_chacha20_random_u32(val)
	u_int32_t *val;
{
	_chacha20_stir_if_needed(sizeof(*val));
	if (rs_have < sizeof(*val)) {
		_chacha20_rekey(NULL, 0);
	}
	memcpy(val, rs_buf + sizeof(rs_buf) - rs_have, sizeof(*val));
	memset(rs_buf + sizeof(rs_buf) - rs_have, 0, sizeof(*val));
	rs_have -= sizeof(*val);
}

/*
 * sysctl kern.urandom
 *
 *	Independent uniform random 32-bit integer.  Read-only.
 */
int
sysctl_urandom(oldp, oldlenp, newp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
{
	int rnd;

	_chacha20_random_buf(&rnd, sizeof(rnd));
	return (sysctl_rdint(oldp, oldlenp, newp, rnd));
}

/*
 * sysctl kern.arandom
 *
 *	Independent uniform random bytes, up to 256 bytes.  Read-only.
 */
int
sysctl_arandom(oldp, oldlenp, newp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
{
	u_int8_t rnd[256];

	_chacha20_random_buf(rnd, sizeof(rnd));
	return (sysctl_rdint(oldp, oldlenp, newp, rnd));
}
