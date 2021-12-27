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
#include <sys/queue.h>
#include <sys/file.h>

#include <miscfs/specfs/specdev.h>

#include <devel/vm/include/vm_swap.h>

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
	 * Initialize swapdev drum
	 */
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

void
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

void
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

/* returns a swapdev from swdevt array if not null */
struct swapdev *
sw_swapdev(index)
	int index;
{
	register struct swapdev *sdp;
	register struct swdevt 	*swp;

	swp = &swdevt[index];
	if(swp) {
		sdp = swp->sw_swapdev;
		if(sdp) {
			return (sdp);
		}
	}
	return (NULL);
}

/*
 * System call swapon(name) enables swapping on device name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this device.
 */
struct swapon_args {
	syscallarg(char)	*name;
};

/* ARGSUSED */
int
swapon(p, uap, retval)
	struct proc *p;
	struct swapon_args *uap;
	int *retval;
{
	register struct vnode *vp;
	register struct swdevt *sp;
	dev_t 	dev;
	int 	error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, uap->name, p);
	if (error == namei(&nd)) {
		return (error);
	}
	vp = nd.ni_vp;
	if (vp->v_type != VBLK) {
		vrele(vp);
		return (ENOTBLK);
	}
	dev = (dev_t)vp->v_rdev;
	if (major(dev) >= nblkdev) {
		vrele(vp);
		return (ENXIO);
	}
	for (sp = &swdevt[0]; sp->sw_dev != NODEV; sp++) {
		if (sp->sw_dev == dev) {
			if (sp->sw_flags & SW_FREED) {
				vrele(vp);
				return (EBUSY);
			}
			sp->sw_vp = vp;
			if (error == swfree(p, sp - swdevt)) {
				vrele(vp);
				return (error);
			}
			if(error == swapdrum_on(p, sp)) {
				vrele(vp);
				return (error);
			}
			return (0);
		}
	}
	vrele(vp);
	return (EINVAL);
}

/*
 * System call swapoff(name) disables swapping on device name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this device.
 */
struct swapoff_args {
	syscallarg(char) *name;
};

int
swapoff(p, uap, retval)
	struct proc *p;
	struct swapoff_args *uap;
	int *retval;
{
	register struct vnode *vp;
	register struct swdevt *sp;
	dev_t 	dev;
	int 	error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, uap->name, p);
	if (error == namei(&nd)) {
		return (error);
	}
	for (sp = &swdevt[0]; sp->sw_dev != NODEV; sp++) {
		if (sp->sw_dev == dev) {
			if(error == swapdrum_off(p, sp)) {
				return (error);
			}
			return (0);
		}
	}

	return (0);
}

int
swalloc(index, lessok)
	int index;
	bool_t 	lessok;
{
	register struct swdevt 	*sp;
	int npages, error;

	sp = &swdevt[index];
	npages = MAXPHYS >> PAGE_SHIFT;

	error = vm_swap_alloc(sp, &npages, lessok);

	return (error);
}

void
swfree(startslot, nslots)
	int startslot;
	int nslots;
{
	vm_swap_free(startslot, nslots);
}

struct swdevt swdevt[] = {
		/* dev, flags, nblks, inuse */
		{ 1, 0,	0, 0 },
		{ NODEV, 1,	0, 0 },
};

void
swapconf()
{
	register struct swdevt 	*swp;
	register int nblks;

	for (swp = swdevt; swp->sw_dev != NODEV; swp++)	{
		if ( (u_int)swp->sw_dev >= nblkdev ){
			break;
		}

		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks = (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (nblks != -1 && (swp->sw_nblks == 0 || swp->sw_nblks > nblks)) {
				swp->sw_nblks = nblks;
			}
			swp->sw_inuse = 0;
		}
	}

	if (dumplo == 0 && bdevsw[major(dumpdev)].d_psize) {
		dumplo = (*bdevsw[major(dumpdev)].d_psize)(dumpdev) - (Maxmem * NBPG/512);
	}
	if (dumplo < 0) {
		dumplo = 0;
	}
}
