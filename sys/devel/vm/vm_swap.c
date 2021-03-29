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
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/dmap.h>		/* XXX */
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/file.h>

#include <miscfs/specfs/specdev.h>

#include <devel/vm/include/vm_swap.h>

#define swdevt_member(swp, member)				(swp->sw_ ## member)
#define swdevt_find_by_index(member, val)	 	(&swdevt[member].sw_ ## val)
#define swdevt_find_by_dev(dev, member)			(swdevt_find_by_index(dev, member))

const struct swdevsw swap_swdevsw = {
		.s_allocate = swallocate,
		.s_free = swfree,
		.s_create = swcreate,
		.s_destroy = swdestroy,
		.s_read = swread,
		.s_write = swwrite,
		.s_strategy = swstrategy
};

int	nswap, nswdev;
#ifdef SEQSWAP
int	niswdev;		/* number of interleaved swap devices */
int	niswap;			/* size of interleaved swap area */
#endif

void
swapinit()
{
	register int i;
	register struct buf *sp = swbuf;
	register struct proc *p = &proc0;	/* XXX */
	struct swdevt *swp;
	int error;

	/*
	 * Count swap devices, and adjust total swap space available.
	 * Some of the space will not be countable until later (dynamically
	 * configurable devices) and some of the counted space will not be
	 * available until a swapon() system call is issued, both usually
	 * happen when the system goes multi-user.
	 *
	 * If using NFS for swap, swdevt[0] will already be bdevvp'd.	XXX
	 */
#ifdef SEQSWAP
	nswdev = niswdev = 0;
	nswap = niswap = 0;
	swap_sequential(swp, niswdev, niswap);

#else
	nswdev = 0;
	nswap = 0;
	swap_interleaved(swp, nswdev, nswap);
#endif
	if (nswap == 0) {
		printf("WARNING: no swap space found\n");
	} else if (error == swfree(p, 0)) {
		printf("swfree errno %d\n", error); /* XXX */
		panic("swapinit swfree 0");
	}

	/*
	 * Now set up swapdev
	 */
	swp->sw_swapdev = (struct swapdev)rmalloc(&swapmap, sizeof(struct swapdev));
	swapdrum_init(swp);

	/*
	 * Now set up swap buffer headers.
	 */
	bswlist.b_actf = sp;
	for (i = 0; i < nswbuf - 1; i++, sp++) {
		sp->b_actf = sp + 1;
		sp->b_rcred = sp->b_wcred = p->p_ucred;
		LIST_NEXT(sp, b_vnbufs) = NOLIST;
	}
	sp->b_rcred = sp->b_wcred = p->p_ucred;
	LIST_NEXT(sp, b_vnbufs) = NOLIST;
	sp->b_actf = NULL;
}

static void
swap_sequential(swp, niswdev, niswap)
	struct swdevt *swp;
	int	niswdev, niswap;
{
	/*
	 * All interleaved devices must come first
	 */
	for (swp = swdevt; swp->sw_dev != NODEV || swp->sw_vp != NULL; swp++) {
		if (swp->sw_flags & SW_SEQUENTIAL) {
			break;
		}
		niswdev++;
		if (swp->sw_nblks > niswap) {
			niswap = swp->sw_nblks;
		}
	}
	niswap = roundup(niswap, dmmax);
	niswap *= niswdev;
	if (swdevt[0].sw_vp == NULL &&
	    bdevvp(swdevt[0].sw_dev, &swdevt[0].sw_vp)) {
		panic("swapvp");
	}
	/*
	 * The remainder must be sequential
	 */
	for ( ; swp->sw_dev != NODEV; swp++) {
		if ((swp->sw_flags & SW_SEQUENTIAL) == 0) {
			panic("binit: mis-ordered swap devices");
		}
		nswdev++;
		if (swp->sw_nblks > 0) {
			if (swp->sw_nblks % dmmax) {
				swp->sw_nblks -= (swp->sw_nblks % dmmax);
			}
			nswap += swp->sw_nblks;
		}
	}
	nswdev += niswdev;
	if (nswdev == 0) {
		panic("swapinit");
	}
	nswap += niswap;
}

static void
swap_interleaved(swp, nswdev, nswap)
	struct swdevt *swp;
	int	nswdev, nswap;
{
	for (swp = swdevt; swp->sw_dev != NODEV || swp->sw_vp != NULL; swp++) {
		nswdev++;
		if (swp->sw_nblks > nswap) {
			nswap = swp->sw_nblks;
		}
	}
	if (nswdev == 0) {
		panic("swapinit");
	}
	if (nswdev > 1) {
		nswap = ((nswap + dmmax - 1) / dmmax) * dmmax;
	}
	nswap *= nswdev;
	if (swdevt[0].sw_vp == NULL && bdevvp(swdevt[0].sw_dev, &swdevt[0].sw_vp)) {
		panic("swapvp");
	}
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
swcreate(p, index)
	struct proc *p;
	int index;
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

int
vm_swap_put(swslot, ppsp, npages, flags)
	struct vm_page **ppsp;
	int swslot, npages, flags;
{
	int error;
	error = vm_swap_io();

	return (error);
}

int
vm_swap_get(page, swslot, flags)
	struct vm_page *page;
	int swslot, flags;
{
	int error;
	return (error);
}
