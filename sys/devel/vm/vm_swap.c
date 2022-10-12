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
	 * Initialize swapdrum dev
	 */
	swapdrum_init(swp);

	/*
	 * Now set up swap buffer headers.
	 */
	vm_swapbuf_init(sp, p);
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

/* enable swapping on device via swapdrum_on */
int
swapon(p, index)
	struct proc *p;
	int 	index;
{
	register struct swdevt 	*swp;

	swp = &swdevt[index];
	return (swapdrum_on(p, swp));
}

/* disable swapping on device via swapdrum_off */
int
swapoff(p, index)
	struct proc *p;
	int 	index;
{
	register struct swdevt 	*swp;

	swp = &swdevt[index];
	return (swapdrum_off(p, swp));
}

int
swalloc(index, nslots, lessok)
	int 	index, *nslots;
	bool_t 	lessok;
{
	register struct swdevt 	*swp;

	swp = &swdevt[index];
	return (vm_swap_alloc(swp, nslots, lessok));
}

int
swfree(p, index, nslots)
	struct proc *p;
	int index, nslots;
{
	register struct swdevt 	*swp;
	register struct swapdev *sdp;
	register struct vnode 	*vp;
	const struct bdevsw		*bdev;
	dev_t dev;
	register swblk_t vsbase, dvbase;
	register long blk;
	register int nblks, npages, startslot;
	int error, perdev;

	swp = &swdevt[index];
	sdp = swp->sw_swapdev;
	vp = swp->sw_vp;
	if (error == VOP_OPEN(vp, FREAD|FWRITE, p->p_ucred, p)) {
		return (error);
	}
	swp->sw_flags |= SW_FREED;
	nblks = swp->sw_nblks;
	npages = dbtob(nblks) >> PAGE_SHIFT;
	if (nblks <= 0) {
		perdev = nswap / nswdev;
		dev = swp->sw_dev;
		bdev = bdevsw_lookup(dev);
		if (bdev->d_psize == 0 || (nblks = (*bdev->d_psize)(dev)) == -1) {
			(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
			swp->sw_flags &= ~SW_FREED;
			return (ENXIO);
		}
#ifdef SEQSWAP
		if (index < niswdev) {
			perdev = niswap / niswdev;
			if (nblks > perdev) {
				nblks = perdev;
			}
		} else {
			if (nblks % dmmax) {
				nblks -= (nblks % dmmax);
			}
			nswap += nblks;
		}
#else
		if (nblks > perdev) {
			nblks = perdev;
		}
#endif
		swp->sw_nblks = nblks;
		npages = dbtob(nblks) >> PAGE_SHIFT;
	}

	if (nblks == 0) {
		if (swap_sanity(nblks, npages) == 0) {
			startslot = swap_search(swp, sdp, nblks, npages);
			vm_swap_free(startslot, nslots);
		}
		(void) VOP_CLOSE(vp, FREAD|FWRITE, p->p_ucred, p);
		swp->sw_flags &= ~SW_FREED;
		return (0);
	}
	/*
	 * 1st: free swap extents.
	 * 2nd: free swapmap.
	 */
//	startslot = swap_search(swp, sdp);
//	vm_swap_free(startslot, nslots);

#ifdef SEQSWAP
	if (swp->sw_flags & SW_SEQUENTIAL) {
		register struct swdevt *swp;

		blk = niswap;
		for (swp = &swdevt[niswdev]; swp != swp; swp++) {
			blk += swp->sw_nblks;
		}
		rmfree(swapmap, nblks, blk);
		return (0);
	}
#endif
	for (dvbase = 0; dvbase < nblks; dvbase += dmmax) {
		blk = nblks - dvbase;

#ifdef SEQSWAP
		if ((vsbase = index*dmmax + dvbase*niswdev) >= niswap) {
			panic("swfree");
		}
#else
		if ((vsbase = index * dmmax + dvbase * nswdev) >= nswap) {
			panic("swfree");
		}
#endif
		if (blk > dmmax) {
			blk = dmmax;
		}
		if (vsbase == 0) {
			/*
			 * First of all chunks... initialize the swapmap.
			 * Don't use the first cluster of the device
			 * in case it starts with a label or boot block.
			 */
			rminit(swapmap, blk - ctod(CLSIZE), vsbase + ctod(CLSIZE), "swap",
			M_SWAPMAP, nswapmap);
		} else if (dvbase == 0) {
			/*
			 * Don't use the first cluster of the device
			 * in case it starts with a label or boot block.
			 */
			rmfree(swapmap, blk - ctod(CLSIZE), vsbase + ctod(CLSIZE));
		} else {
			rmfree(swapmap, blk, vsbase);
		}
	}
	return (0);
}

/* find startslot for /dev/drum */
int
swap_search(swp, sdp, nblks, npages)
	struct swdevt 	*swp;
	struct swapdev 	*sdp;
	int nblks, npages;
{
	int pageno, startslot;

	pageno = btodb(npages << PAGE_SHIFT);
	if (pageno == 0) {
		for(pageno = 1; pageno <= npages; pageno++) {
			if (pageno >= sdp->swd_drumoffset && pageno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
				startslot = sdp->swd_drumoffset;
				break;
			}
		}
	} else {
		if (pageno >= sdp->swd_drumoffset && pageno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
			startslot = sdp->swd_drumoffset;
			break;
		}
	}
	return (startslot);
}

/* sanity check npages and nblks align */
int
swap_sanity(nblks, npages)
	int nblks, npages;
{
	long blk;
	swblk_t dvbase;
	int i, page;
	for (dvbase = 0; dvbase < nblks; dvbase += dmmax) {
		blk = nblks - dvbase;
		break;
	}
    for(i = 0; i <= npages; i++) {
        if(i != 0) {
            page = i;
            break;
        }
    }

    if(btodb(page << PAGE_SHIFT) == blk) {
    	return (0);
    }
    return (1);
}

/* to be placed machine autoconf.c */
struct swdevt swdevt[] = {
		/* dev, flags, nblks, inuse */
		{ 1, 0,	0, 0 },
		{ NODEV, 1,	0, 0 },
};
