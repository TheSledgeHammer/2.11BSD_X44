/*	$NetBSD: mem.c,v 1.59 2003/08/07 16:27:55 agc Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mem.c	8.3 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)mem.c	8.3 (Berkeley) 1/12/94
 */

/*
 * Memory special file
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/lock.h>

#include <machine/cpu.h>

#include <vm/include/vm_param.h>
#include <vm/include/vm_prot.h>
#include <vm/include/pmap.h>

#define	DEV_MEM		0	/* minor device 0 is physical memory */
#define	DEV_KMEM	1	/* minor device 1 is kernel memory */
#define	DEV_NULL	2	/* minor device 2 is EOF/rathole */
#define DEV_FULL	11	/* minor device 11 is '\0'/ENOSPC */
#define	DEV_ZERO	12	/* minor device 12 is '\0'/rathole */
#define	DEV_IO		14

extern char *cvmmap;     /* poor name! */
caddr_t zeropage;

dev_type_open(mmopen);
dev_type_read(mmrw);
dev_type_ioctl(mmioctl);
dev_type_mmap(mmmmap);

const struct cdevsw mm_cdevsw = {
		.d_open = mmopen,
		.d_close = nullclose,
		.d_read = mmrw,
		.d_write = mmrw,
		.d_ioctl = mmioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = mmmmap,
		.d_discard = nodiscard,
		.d_kqfilter = nokqfilter,
		.d_type = D_OTHER,
};

/*ARGSUSED*/
int
mmopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	switch (minor(dev)) {
	/* This is done by i386_iopl(3) now. */
	case DEV_IO:
	default:
		break;
	}
	return (0);
}

/*ARGSUSED*/
int
mmrw(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	register vm_offset_t o, v;
	register int c;
	register struct iovec *iov;
	int error = 0;
	static int physlock;
	vm_prot_t prot;

	if (minor(dev) == DEV_MEM) {
		/* lock against other uses of shared vmmap */
		while (physlock > 0) {
			physlock++;
			error = tsleep((caddr_t) & physlock, PZERO | PCATCH, "mmrw", 0);
			if (error)
				return (error);
		}
		physlock = 1;
	}
	while (uio->uio_resid > 0 && !error) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {
		case DEV_MEM:
			v = uio->uio_offset;
			prot = uio->uio_rw == UIO_READ ? VM_PROT_READ : VM_PROT_WRITE;
			pmap_enter(kernel_pmap, (vm_offset_t) cvmmap, trunc_page(v), prot, TRUE);
			pmap_update();
			o = uio->uio_offset & PGOFSET;
			c = min(uio->uio_resid, (int) (PAGE_SIZE - o));
			error = uiomove((caddr_t) cvmmap + o, c, uio);
			pmap_remove(kernel_pmap, (vm_offset_t) cvmmap, (vm_offset_t) cvmmap + PAGE_SIZE);
			pmap_update();
			break;

		case DEV_KMEM:
			v = uio->uio_offset;
			c = min(iov->iov_len, MAXPHYS);
			if (!kernacc((caddr_t) v, c, uio->uio_rw == UIO_READ ? B_READ : B_WRITE))
				return (EFAULT);
			error = uiomove((caddr_t) v, c, uio);
			break;

		case DEV_NULL:
			if (uio->uio_rw == UIO_WRITE)
				uio->uio_resid = 0;
			return (0);

		case DEV_ZERO:
			if (uio->uio_rw == UIO_WRITE) {
				uio->uio_resid = 0;
				return (0);
			}
			if (zeropage == NULL) {
				zeropage = (caddr_t) malloc(PAGE_SIZE, M_TEMP, M_WAITOK);
				memset(zeropage, 0, PAGE_SIZE);
			}
			c = min(iov->iov_len, PAGE_SIZE);
			error = uiomove(zeropage, c, uio);
			break;

		default:
			return (ENXIO);
		}
	}
	if (minor(dev) == DEV_MEM) {
		if (physlock > 1)
			wakeup((caddr_t) & physlock);
		physlock = 0;
	}
	return (error);
}

caddr_t
mmmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	struct proc *p = curproc;	/* XXX */

	/*
	 * /dev/mem is the only one that makes sense through this
	 * interface.  For /dev/kmem any physaddr we return here
	 * could be transient and hence incorrect or invalid at
	 * a later time.  /dev/null just doesn't make any sense
	 * and /dev/zero is a hack that is handled via the default
	 * pager in mmap().
	 */
	if (minor(dev) != DEV_MEM)
		return ((caddr_t)-1);

	if ((u_int) off > ctob(physmem) && suser1(p->p_ucred, &p->p_acflag) != 0)
		return ((caddr_t)-1);
	return ((caddr_t)i386_btop((u_int) off));
}

int
mmioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (minor(dev)) {
	default:
		switch (cmd) {
		case FIONBIO:	/* We never block anyway */
			return 0;
		case FIOSETOWN:
		case FIOGETOWN:
		case TIOCGPGRP:
		case TIOCSPGRP:
			return ENOTTY;
		case FIOASYNC:
			if ((*(int *)data) == 0)
				return 0;
			/*FALLTHROUGH*/
		}
		return EOPNOTSUPP;
	}
}
