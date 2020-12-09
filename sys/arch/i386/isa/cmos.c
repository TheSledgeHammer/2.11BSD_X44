/*	$NetBSD: cmos.c,v 1.12 2015/08/20 14:40:16 christos Exp $	*/

/*
 * Copyright (C) 2003 JONE System Co., Inc.
 * All right reserved.
 *
 * Copyright (C) 1999, 2000, 2001, 2002 JEPRO Co., Ltd.
 * All right reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * Copyright (c) 2007 David Young.  All right reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: cmos.c,v 1.12 2015/08/20 14:40:16 christos Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/lock.h>
#include <sys/user.h>

#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/mc146818reg.h>
#include <dev/isa/rtc.h>

#define	CMOS_SUM		32
#define	CMOS_BIOSSPEC	34	/* start of BIOS-specific configuration data */

#define	NVRAM_SUM		(MC_NVRAM_START + CMOS_SUM)
#define	NVRAM_BIOSSPEC	(MC_NVRAM_START + CMOS_BIOSSPEC)

#define	CMOS_SIZE		NVRAM_BIOSSPEC

dev_type_open(cmos_open);
dev_type_close(cmos_close);
dev_type_read(cmos_read);
dev_type_write(cmos_write);

static int 	cmos_initialized;
static void cmos_sum(uint8_t *, int, int, int);
static int	cmos_check(void);

const struct cdevsw cmos_cdevsw = {
		.d_open = 	cmos_open,
		.d_close = 	cmos_close,
		.d_read = 	cmos_read,
		.d_write = 	cmos_write,
		.d_type = 	D_OTHER
};

static struct lock_object *cmos_lock;
static uint8_t cmos_buf[CMOS_SIZE];

void
cmos_attach(int num)
{
	if (num > 1)
		return;
	if(cmos_initialized || cmos_check) {
#ifdef CMOS_DEBUG
		printf("cmos: initialized\n");
#endif
		cmos_initialized = 1;
	} else {
		printf("cmos: invalid checksum\n");
	}

	simple_lock_init(&cmos_lock, "cmos_lock");
}

int
cmos_open(dev_t dev, int flags, int ifmt, struct proc *p)
{
	simple_lock(&cmos_lock);
	if ((minor(dev) != 0) || (!cmos_initialized)) {
		simple_unlock(&cmos_lock);
		return (ENXIO);
	}
	if ((flags & FWRITE)) {
		simple_unlock(&cmos_lock);
		return (EPERM);
	}

	simple_unlock(&cmos_lock);
	return (0);
}

int
cmos_close(dev_t dev, int flags, int ifmt, struct proc *p)
{
	return (0);
}

static void
cmos_fetch(void)
{
	int i, s;
	uint8_t *p;

	p = cmos_buf;
	s = splclock();
	for (i = 0; i < CMOS_SIZE; i++)
		*p++ = mc146818_read(NULL, i);
	splx(s);
}

int
cmos_read(dev_t dev, struct uio *uio, int ioflag)
{
	int error;

	if (uio->uio_offset + uio->uio_resid > CMOS_SIZE)
		return EINVAL;

	simple_lock(&cmos_lock);
	cmos_fetch();
	error = uiomove(cmos_buf + uio->uio_offset, uio->uio_resid, uio);
	simple_unlock(&cmos_lock);

	return error;
}

int
cmos_write(dev_t dev, struct uio *uio, int ioflag)
{
	int error = 0, i, s;

	if (uio->uio_offset + uio->uio_resid > CMOS_SIZE)
		return EINVAL;

	simple_lock(&cmos_lock);
	cmos_fetch();
	error = uiomove(cmos_buf + uio->uio_offset, uio->uio_resid, uio);
	if (error == 0) {
		cmos_sum(cmos_buf, NVRAM_DISKETTE, NVRAM_SUM, NVRAM_SUM);
		s = splclock();
		for (i = NVRAM_DISKETTE; i < CMOS_SIZE; i++)
			mc146818_write(NULL, i, cmos_buf[i]);
		splx(s);
	}
	simple_unlock(&cmos_lock);

	return error;
}

static void
cmos_sum(uint8_t *p, int from, int to, int offset)
{
	int i;
	uint16_t sum;

#ifdef CMOS_DEBUG
	printf("%s: from %d to %d and store %d\n", __func__, from, to, offset);
#endif

	sum = 0;
	for (i = from; i < to; i++)
		sum += p[i];
	p[offset] = sum >> 8;
	p[offset + 1] = sum & 0xff;
}

/*
 * check whether the CMOS layout is "standard"-like (ie, not PS/2-like),
 * to be called at splclock()
 */
static int
cmos_check(void)
{
	int i;
	unsigned short cksum = 0;

	for (i = 0x10; i <= 0x2d; i++)
		cksum += mc146818_read(NULL, i); /* XXX softc */

	return (cksum == (mc146818_read(NULL, 0x2e) << 8) + mc146818_read(NULL, 0x2f));
}

#if NMCA > 0
/*
 * Check whether the CMOS layout is PS/2 like, to be called at splclock().
 */
static int cmoscheckps2(void);
static int
cmoscheckps2(void)
{
#if 0
	/* Disabled until I find out the CRC checksum algorithm IBM uses */
	int i;
	unsigned short cksum = 0;

	for (i = 0x10; i <= 0x31; i++)
		cksum += mc146818_read(NULL, i); /* XXX softc */

	return (cksum == (mc146818_read(NULL, 0x32) << 8) + mc146818_read(NULL, 0x33));
#else
	/* Check 'incorrect checksum' bit of IBM PS/2 Diagnostic Status Byte */
	return ((mc146818_read(NULL, RTC_DIAG) & (1<<6)) == 0);
#endif
}
#endif /* NMCA > 0 */
