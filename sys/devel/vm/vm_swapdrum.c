/*	$NetBSD: uvm_swap.c,v 1.27.2.2 2000/01/08 18:43:28 he Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997 Matthew R. Green
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: NetBSD: vm_swap.c,v 1.52 1997/12/02 13:47:37 pk Exp
 * from: Id: uvm_swap.c,v 1.1.2.42 1998/02/02 20:38:06 chuck Exp
 */
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
#include <sys/extent.h>
#include <sys/file.h>

#include <miscfs/specfs/specdev.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_swap.h>

/*
 * We keep a of pool vndbuf's and vndxfer structures.
 */
static struct vndxfer 			*vndxfer_pool;
static struct vndbuf 			*vndbuf_pool;

/*
 * local variables
 */
static struct extent 			*swapextent;		/* controls the mapping of /dev/drum */
SIMPLEQ_HEAD(swapbufhead, swapbuf);
static struct swapbuf 			*swapbuf_pool;

/* list of all active swap devices [by priority] */
LIST_HEAD(swap_priority, swappri);
static struct swap_priority 	swap_priority;

void
swapdrum_init(swp)
	struct swdevt *swp;
{
	register struct swapdev *sdp;

	sdp = (struct swapdev *)rmalloc(&swapmap, sizeof(struct swapdev *));
	swp->sw_swapdev = sdp;

	/*
	 * first, init the swap list, its counter, and its lock.
	 * then get a handle on the vnode for /dev/drum by using
	 * the its dev_t number ("swapdev", from MD conf.c).
	 */
	LIST_INIT(&swap_priority);
	cnt.v_nswapdev = 0;
	simple_lock_init(&swap_data_lock);

	if (bdevvp(swapdev, &swapdev_vp)) {
		panic("vm_swap_init: can't get vnode for swap device");
	}

	/*
	 * create swap block resource map to map /dev/drum.   the range
	 * from 1 to INT_MAX allows 2 gigablocks of swap space.  note
	 * that block 0 is reserved (used to indicate an allocation
	 * failure, or no allocation).
	 */
	swapextent = extent_create("swapextent", 1, INT_MAX, &swapmap.m_type, 0, 0, EX_NOWAIT);
	if (swapextent == 0) {
		panic("swapinit: extent_create failed");
	}
	&swapbuf_pool = (struct swapbuf)rmalloc(&swapmap, sizeof(struct swapbuf));
	&vndxfer_pool = (struct vndxfer)rmalloc(&swapmap, sizeof(struct vndxfer));
	&vndbuf_pool = (struct vndbuf)rmalloc(&swapmap, sizeof(struct vndbuf));
}

/*
 * swaplist functions: functions that operate on the list of swap
 * devices on the system.
 */

/*
 * swaplist_insert: insert swap device "sdp" into the global list
 *
 * => caller must hold both swap_syscall_lock and uvm.swap_data_lock
 * => caller must provide a newly malloc'd swappri structure (we will
 *	FREE it if we don't need it... this it to prevent malloc blocking
 *	here while adding swap)
 */
void
swaplist_insert(sdp, newspp, priority)
	struct swapdev *sdp;
	struct swappri *newspp;
	int priority;
{
	struct swappri *spp, *pspp;
	/*
	 * find entry at or after which to insert the new device.
	 */
	for (pspp = NULL, spp = LIST_FIRST(&swap_priority); spp != NULL; spp = LIST_NEXT(spp, spi_swappri)) {
		if (priority <= spp->spi_priority) {
			break;
		}
		pspp = spp;
	}

	/*
	 * new priority?
	 */
	if (spp == NULL || spp->spi_priority != priority) {
		spp = newspp;  /* use newspp! */

		spp->spi_priority = priority;
		CIRCLQ_INIT(&spp->spi_swapdev);

		if (pspp) {
			LIST_INSERT_AFTER(pspp, spp, spi_swappri);
		} else {
			LIST_INSERT_HEAD(&swap_priority, spp, spi_swappri);
		}
	} else {
		/* we don't need a new priority structure, free it */
		rmfree(&swapmap, newspp, sizeof(*newspp));
	}
	sdp->swd_priority = priority;
	CIRCLEQ_INSERT_TAIL(&spp->spi_swapdev, sdp, swd_next);
	cnt.v_nswapdev++;
}

/*
 * swaplist_find: find and optionally remove a swap device from the
 *	global list.
 *
 * => caller must hold both swap_syscall_lock and uvm.swap_data_lock
 * => we return the swapdev we found (and removed)
 */
struct swapdev *
swaplist_find(vp, remove)
	struct vnode *vp;
	boolean_t remove;
{
	struct swapdev *sdp;
	struct swappri *spp;

	/*
	 * search the lists for the requested vp
	 */
	LIST_FOREACH(spp, &swap_priority, spi_swappri) 	{
		CIRCLEQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp->swd_swdevt->sw_vp != vp) {			/* XXX: correct */
				continue;
			}
			if (remove) {
				CIRCLEQ_REMOVE(&spp->spi_swapdev, sdp, swd_next);
				cnt.v_nswapdev--;
			}
			return (sdp);
		}
	}
	return (NULL);
}

/*
 * swaplist_trim: scan priority list for empty priority entries and kill
 *	them.
 *
 * => caller must hold both swap_syscall_lock and uvm.swap_data_lock
 */
void
swaplist_trim(void)
{
	struct swappri *spp, *nextspp;

	LIST_FOREACH_SAFE(spp, &swap_priority, spi_swappri, nextspp) {
		if (!CIRCLEQ_EMPTY(&spp->spi_swapdev)) {
			continue;
		}
		LIST_REMOVE(spp, spi_swappri);
		rmfree(&swapmap, spp, sizeof(*spp));
	}
}

/*
 * swapdrum_add: add a "swapdev"'s blocks into /dev/drum's area.
 *
 * => caller must hold swap_syscall_lock
 * => uvm.swap_data_lock should be unlocked (we may sleep)
 */
static void
swapdrum_add(sdp, npages)
	struct swapdev *sdp;
	int				npages;
{
	u_long result;

	if (extent_alloc(swapextent, npages, EX_NOALIGN, EX_NOBOUNDARY, EX_WAITOK, &result)) {
		panic("swapdrum_add");
	}

	sdp->swd_drumoffset = result;
	sdp->swd_drumsize = npages;
}

/*
 * swapdrum_getsdp: given a page offset in /dev/drum, convert it back
 *	to the "swapdev" that maps that section of the drum.
 *
 * => each swapdev takes one big contig chunk of the drum
 * => caller must hold uvm.swap_data_lock
 */
struct swapdev *
swapdrum_getsdp(pgno)
	int pgno;
{
	struct swapdev *sdp;
	struct swappri *spp;

	for (spp = LIST_FIRST(swap_priority); spp != NULL; spp = LIST_NEXT(spp, spi_swappri)) {
		for (sdp = CIRCLEQ_FIRST(spp->spi_swapdev); sdp != (void *)&spp->spi_swapdev; sdp = CIRCLEQ_NEXT(sdp, swd_next)) {
			if (pgno >= sdp->swd_drumoffset && pgno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
				return (sdp);
			}
		}
	}
	return (NULL);
}

int
swapdrum_on(p, sp)
	struct proc *p;
	struct swdevt *sp;
{
	struct swapdev *sdp;
	static int count = 0;	/* static */
	struct vnode *vp;
	int error, npages, nblks, size;
	long addr;
	u_long result;
#ifdef SWAP_TO_FILES
	struct vattr va;
#endif
	dev_t dev;
	char *name;

	/*
	 * we want to enable swapping on sdp. the swd_vp contains
	 * the vnode we want (locked and ref'd), and the swd_dev
	 * contains the dev_t of the file, if it a block device.
	 */
	vp = sp->sw_vp;
	dev = sp->sw_dev;
	sdp = sp->sw_swapdev;

	/*
	 * open the swap file (mostly useful for block device files to
	 * let device driver know what is up).
	 *
	 * we skip the open/close for root on swap because the root
	 * has already been opened when root was mounted (mountroot).
	 */
	if ((error = VOP_OPEN(vp, FREAD|FWRITE, p->p_ucred, p))) {
		return (error);
	}

	/*
	 * we now need to determine the size of the swap area.   for
	 * block specials we can call the d_psize function.
	 * for normal files, we must stat [get attrs].
	 *
	 * we put the result in nblks.
	 * for normal files, we also want the filesystem block size
	 * (which we get with statfs).
	 */
	switch (vp->v_type) {
	case VBLK:
		if (bdevsw[major(dev)].d_psize == 0 || (nblks = (*bdevsw[major(dev)].d_psize)(dev)) == -1) {
			(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
			return (ENXIO);
		}
		break;
#ifdef SWAP_TO_FILES
	case VREG:
		if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p))) {
			(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
			return (error);
		}
		nblks = (int)btodb(va.va_size);
		if ((error = VFS_STATFS(vp->v_mount, &vp->v_mount->mnt_stat, p)) != 0) {
			(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
			return (error);
		}
		sdp->swd_bsize = vp->v_mount->mnt_stat.f_iosize;
		/*
		 * limit the max # of outstanding I/O requests we issue
		 * at any one time.   take it easy on NFS servers.
		 */
		break;
#endif
	default:
		(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
		return (ENXIO);
	}

	/*
	 * save nblocks in a safe place and convert to pages.
	 */
	sp->sw_nblks = nblks;
	npages = dbtob((u_int64_t)nblks) >> PAGE_SHIFT;

	/*
	 * for block special files, we want to make sure that leave
	 * the disklabel and bootblocks alone, so we arrange to skip
	 * over them (randomly choosing to skip PAGE_SIZE bytes).
	 * note that because of this the "size" can be less than the
	 * actual number of blocks on the device.
	 */
	if (vp->v_type == VBLK) {
		/* we use pages 1 to (size - 1) [inclusive] */
		size = npages - 1;
		addr = 1;
	} else {
		/* we use pages 0 to (size - 1) [inclusive] */
		size = npages;
		addr = 0;
	}

	/*
	 * make sure we have enough blocks for a reasonable sized swap
	 * area.   we want at least one page.
	 */
	if (size < 1) {
		(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
		return (EINVAL);
	}

	/*
	 * now we need to allocate an extent to manage this swap device
	 */
	name = rmalloc(&swapmap, 12);
	sprintf(name, "swap0x%04x", count++);

	/* note that extent_create's 3rd arg is inclusive, thus "- 1" */
	sdp->swd_ex = extent_create(name, 0, npages - 1, &swapmap.m_type, 0, 0, EX_WAITOK);
	/* allocate the `saved' region from the extent so it won't be used */
	if (addr) {
		if (extent_alloc_region(sdp->swd_ex, 0, addr, EX_WAITOK)) {
			panic("disklabel region");
		}
		sdp->swd_npginuse += addr;
		simple_lock(&swap_data_lock);
		cnt.v_swpginuse += addr;
		cnt.v_swpgonly += addr;
		simple_unlock(&swap_data_lock);
	}

	/* check if vp == rootvp via swap_miniroot */
	error = swap_miniroot(sp, sdp, vp, npages);
	if(error != 0) {
		return (error);
	}

	/*
	 * add a ref to vp to reflect usage as a swap device.
	 */
	vref(vp);

	/*
	 * now add the new swapdev to the drum and enable.
	 */
	simple_lock(&swap_data_lock);
	swapdrum_add(sdp, npages);
	sdp->swd_npages = npages;
	sp->sw_flags &= ~SW_FAKE;	/* going live */
	sp->sw_flags |= (SW_INUSE|SW_ENABLE);
	simple_unlock(&swap_data_lock);
	cnt.v_swpages += npages;
	/*
	 * add anon's to reflect the swap space we added
	 */
	vm_anon_add(size);

	return (0);

bad:
	if (sdp->swd_ex) {
		extent_destroy(sdp->swd_ex);
	}
	if (vp != rootvp) {
		(void)VOP_CLOSE(vp, FREAD|FWRITE, p->p_ucred, p);
	}
	return (error);
}

int
swapdrum_off(p, sp)
	struct proc 	*p;
	struct swdevt 	*sp;
{
	struct swapdev 	*sdp;
	char			*name;

	sdp = sp->sw_swapdev;

	/* turn off the enable flag */
	sp->sw_flags &= ~SW_ENABLE;

	/*
	 * free all resources!
	 */
	extent_free(swapextent, sdp->swd_drumoffset, sdp->swd_drumsize, EX_WAITOK);
	name = sdp->swd_ex->ex_name;
	extent_destroy(sdp->swd_ex);
	rmfree(&swapmap, sizeof(name), name);
	rmfree(&swapmap, sizeof(sdp->swd_ex), (caddr_t)sdp->swd_ex);
	if(sp->sw_vp != rootvp) {
		(void) VOP_CLOSE(sp->sw_vp, FREAD|FWRITE, p->p_ucred, p);
	}
	if(sp->sw_vp) {
		vrele(sp->sw_vp);
	}
	rmfree(&swapmap, sizeof(sdp), (caddr_t)sdp);
	return (0);
}

int
swap_miniroot(p, sp, sdp, vp, npages)
	struct proc 	*p;
	struct swdevt 	*sp;
	struct swapdev 	*sdp;
	struct vnode 	*vp;
	int npages;
{
	/*
	 * if the vnode we are swapping to is the root vnode
	 * (i.e. we are swapping to the miniroot) then we want
	 * to make sure we don't overwrite it.   do a statfs to
	 * find its size and skip over it.
	 */
	if (vp == rootvp) {
		struct mount *mp;
		struct statfs *stp;
		int rootblocks, rootpages;

		mp = rootvnode->v_mount;
		stp = &mp->mnt_stat;
		rootblocks = stp->f_blocks * btodb(stp->f_bsize);
		rootpages = round_page(dbtob(rootblocks)) >> PAGE_SHIFT;
		if (rootpages > npages) {
			panic("swap_on: miniroot larger than swap?");
		}
		if (extent_alloc_region(sdp->swd_ex, addr, rootpages, EX_WAITOK)) {
			panic("swap_on: unable to preserve miniroot");
		}
		simple_lock(&swap_data_lock);
		sdp->swd_npginuse += (rootpages - addr);
		cnt.v_swpginuse += (rootpages - addr);
		cnt.v_swpgonly += (rootpages - addr);
		simple_unlock(&swap_data_lock);
	}
	if (vp != rootvp) {
		(void) VOP_CLOSE(vp, FREAD|FWRITE, p->p_ucred, p);
		return (ENXIO);
	}
	return (0);
}
