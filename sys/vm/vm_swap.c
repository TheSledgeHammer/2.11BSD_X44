/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)vm_swap.c	8.5 (Berkeley) 2/17/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/dmap.h>		/* XXX */
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/file.h>
#include <sys/sysdecl.h>

#include <miscfs/specfs/specdev.h>

#include <vm/include/vm.h>
#include <vm/include/vm_swap.h>

/*
 * Indirect driver for multi-controller paging.
 */
dev_type_read(swread);
dev_type_write(swwrite);
dev_type_strategy(swstrategy);

const struct bdevsw swap_bdevsw = {
		.d_open = nullopen,
		.d_close = nullclose,
		.d_strategy = swstrategy,
		.d_ioctl = noioctl,
		.d_dump = nodump,
		.d_psize = nosize,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

const struct cdevsw swap_cdevsw = {
		.d_open = nullopen,
		.d_close = nullclose,
		.d_read = swread,
		.d_write = swwrite,
		.d_ioctl = noioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = nommap,
		.d_discard = nodiscard,
		.d_type = D_OTHER,
};

int	nswap, nswdev;
#ifdef SEQSWAP
int	niswdev;		/* number of interleaved swap devices */
int	niswap;			/* size of interleaved swap area */
static void swap_sequential(struct swdevt *, int, int);
#else
static void	swap_interleaved(struct swdevt *, int, int);
#endif
static int 	swap_search(struct swapdev *, int, int);

/*
 * Set up swap devices.
 * Initialize linked list of free swap
 * headers. These do not actually point
 * to buffers, but rather to pages that
 * are being swapped in and out.
 */
void
swapinit(void)
{
	register int i;
	register struct buf *sp;
	register struct proc *p;
	struct swdevt *swp;
	int error;

	sp = swbuf;
	p = &proc0;	/* XXX */

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
	} else if ((error = swfree(p, 0, 1))) {
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
swstrategy(bp)
	register struct buf *bp;
{
	register struct swdevt *sp;
	struct vnode *vp;
	int sz, off, seg, index;

#ifdef GENERIC
	/*
	 * A mini-root gets copied into the front of the swap
	 * and we run over top of the swap area just long
	 * enough for us to do a mkfs and restor of the real
	 * root (sure beats rewriting standalone restor).
	 */
#define	MINIROOTSIZE	4096
	if (rootdev == dumpdev) {
		bp->b_blkno += MINIROOTSIZE;
	}
#endif
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (bp->b_blkno + sz > nswap) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	if (nswdev > 1) {
#ifdef SEQSWAP
		if (bp->b_blkno < niswap) {
			if (niswdev > 1) {
				off = bp->b_blkno % dmmax;
				if (off + sz > dmmax) {
					bp->b_error = EINVAL;
					bp->b_flags |= B_ERROR;
					biodone(bp);
					return;
				}
				seg = bp->b_blkno / dmmax;
				index = seg % niswdev;
				seg /= niswdev;
				bp->b_blkno = seg * dmmax + off;
			} else {
				index = 0;
			}
		} else {
			register struct swdevt *swp;

			bp->b_blkno -= niswap;
			for (index = niswdev, swp = &swdevt[niswdev]; swp->sw_dev != NODEV; swp++, index++) {
				if (bp->b_blkno < swp->sw_nblks) {
					break;
				}
				bp->b_blkno -= swp->sw_nblks;
			}
			if (swp->sw_dev == NODEV || bp->b_blkno+sz > swp->sw_nblks) {
				bp->b_error = swp->sw_dev == NODEV ? ENODEV : EINVAL;
				bp->b_flags |= B_ERROR;
				biodone(bp);
				return;
			}
		}
#else
		off = bp->b_blkno % dmmax;
		if (off + sz > dmmax) {
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		seg = bp->b_blkno / dmmax;
		index = seg % nswdev;
		seg /= nswdev;
		bp->b_blkno = seg * dmmax + off;
#endif
	} else {
		index = 0;
	}
	sp = &swdevt[index];
	if ((bp->b_dev = sp->sw_dev) == NODEV) {
		panic("swstrategy");
	}
	if (sp->sw_vp == NULL) {
		bp->b_error = ENODEV;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	VHOLD(sp->sw_vp);
	swapdrum_strategy(sp, bp, vp);
}

/*
 * Swfree(index) frees the index'th portion of the swap map.
 * Each of the nswdev devices provides 1/nswdev'th of the swap
 * space, which is laid out with blocks of dmmax pages circularly
 * among the devices.
 */
int
swfree(p, index, nslots)
	struct proc *p;
	int index, nslots;
{
	register struct swdevt 	*swp;
	register struct swapdev *sdp;
	register struct vnode 	*vp;
	const struct bdevsw		*bdev;
	register swblk_t vsbase, dvbase;
	register int nblks, npages, startslot;
	register long blk;
	int error, perdev;
	dev_t dev;

	swp = &swdevt[index];
	sdp = swp->sw_swapdev;
	sdp->swd_swdevt = swp;
	vp = swp->sw_vp;
	if ((error = VOP_OPEN(vp, FREAD|FWRITE, p->p_ucred, p))) {
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

	startslot = swap_search(sdp, nblks, npages);
	if (startslot == sdp->swd_drumoffset) {
		vm_swap_free(startslot, nslots);
	}

	if (nblks == 0) {
		(void) VOP_CLOSE(vp, FREAD|FWRITE, p->p_ucred, p);
		swp->sw_flags &= ~SW_FREED;
		return (0);
	}

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
			rmallocate(swapmap, blk - ctod(CLSIZE), vsbase + ctod(CLSIZE), nswapmap);
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

#ifdef SEQSWAP

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

#else

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

#endif

/* find startslot for /dev/drum */
static int
swap_search(sdp, nblks, npages)
	struct swapdev 	*sdp;
	int nblks, npages;
{
	long blk;
	int pageno, startslot;
	swblk_t dvbase;

	if (nblks > 0) {
		for (dvbase = 0; dvbase < nblks; dvbase += dmmax) {
			blk = nblks - dvbase;
			break;
		}
	} else {
		blk = 0;
	}

	pageno = btodb(npages << PAGE_SHIFT);
	if (pageno == 0 && blk == pageno) {
		for(pageno = 1; pageno <= npages; pageno++) {
			if (pageno >= sdp->swd_drumoffset && pageno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
				startslot = sdp->swd_drumoffset;
				break;
			}
		}
	} else {
		if (pageno >= sdp->swd_drumoffset && pageno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
			startslot = sdp->swd_drumoffset;
		}
	}
	return (startslot);
}

int
swread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return (physio(swstrategy, NULL, dev, B_READ, minphys, uio));
}

int
swwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return (physio(swstrategy, NULL, dev, B_WRITE, minphys, uio));
}
