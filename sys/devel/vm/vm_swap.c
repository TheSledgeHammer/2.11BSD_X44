/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Redesign of the vm swap. Taking inspiration from the NetBSD's UVM and 2.11BSD/4.3BSD's vm swap.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/dmap.h>		/* XXX */
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/file.h>

#include <miscfs/specfs/specdev.h>

#include <devel/vm/include/vm_swap.h>

/*
 * We keep a of pool vndbuf's and vndxfer structures.
 */
static struct vndxfer 			vndxfer_pool;
static struct vndbuf 			vndbuf_pool;

/*
 * local variables
 */
static struct extent 			*swapextent;		/* controls the mapping of /dev/drum */

dev_type_read(swread);
dev_type_write(swwrite);

const struct cdevsw swap_cdevsw = {
		.d_open = noopen,
		.d_close = noclose,
		.d_read = swread,
		.d_write = swwrite,
		.d_ioctl = noioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = nomap,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

const struct swapdevsw swap_devsw = {
		.sw_allocate = swallocate,
		.sw_free = swfree,
		.sw_create = swcreate,
		.sw_destroy = swdestroy,
		.sw_read = swread,
		.sw_write = swwrite,
		.sw_strategy = swstrategy
};

void
swapinit()
{

}

/*
 * System call swapon(name) enables swapping on device name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this device.
 */
struct swapon_args {
	char	*name;
};

/* ARGSUSED */
int
swapon(p, uap, retval)
	struct proc *p;
	struct swapon_args *uap;
	int *retval;
{
	return (0);
}

/*
 * System call swapoff(name) disables swapping on device name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this device.
 */
struct swapoff_args {
	char	*name;
};

int
swapoff(p, uap, retval)
	struct proc *p;
	struct swapoff_args *uap;
	int *retval;
{
	return (0);
}

int
swallocate(p, index)
	struct proc *p;
	int index;
{
	return (0);
}

int
swfree(p, index)
	struct proc *p;
	int index;
{
	return (0);
}

int
swcreate()
{
	return (0);
}

int
swdestroy()
{
	return (0);
}

int
swread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (rawread(dev, uio));
}

int
swwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (rawwrite(dev, uio));
}

void
swstrategy(bp)
	register struct buf *bp;
{

}
