/*	$NetBSD: uvm_swap.c,v 1.27.2.2 2000/01/08 18:43:28 he Exp $	*/

/*
 * Copyright (c) 2020 Martin Kelly
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
//#include <sys/buf.h>
//#include <sys/bufq.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/dmap.h>		/* XXX */
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/extent.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/sysdecl.h>

#include <miscfs/specfs/specdev.h>

#include <vm/include/vm.h>
#include <vm/include/vm_swap.h>

/*
* The following two structures are used to keep track of data transfers
* on swap devices associated with regular files.
* NOTE: this code is more or less a copy of vnd.c; we use the same
* structure names here to ease porting..
*/
struct vndxfer {
	struct buf				*vx_bp;			/* Pointer to parent buffer */
	struct swapdev			*vx_sdp;
	int						vx_error;
	int						vx_pending;		/* # of pending aux buffers */
	int						vx_flags;
#define VX_BUSY				1
#define VX_DEAD				2
};

struct vndbuf {
	struct buf				vb_buf;
	struct vndxfer			*vb_xfer;
};

/*
 * local variables
 */
static struct extent 			*swapextent;		/* controls the mapping of /dev/drum */
SIMPLEQ_HEAD(swapbufhead, swapbuf);

/* list of all active swap devices [by priority] */
LIST_HEAD(swap_priority, swappri);
static struct swap_priority 	swap_priority;

int swap_miniroot(struct proc *, struct swapdev *, struct vnode *, int, long);
static void sw_reg_strategy(struct swdevt *, struct buf *, int);
static void sw_reg_start(struct swdevt *);
static void sw_reg_iodone(struct buf *);

void
swapdrum_init(swp)
	struct swdevt *swp;
{
	register struct swapdev *sdp;

	sdp = (struct swapdev *)rmalloc(swapmap, sizeof(struct swapdev *));
	sdp->swd_swdevt = swp;
	swp->sw_swapdev = sdp;

	/*
	 * first, init the swap list, its counter, and its lock.
	 * then get a handle on the vnode for /dev/drum by using
	 * the its dev_t number ("swapdev", from MD conf.c).
	 */
	LIST_INIT(&swap_priority);
	cnt.v_nswapdev = 0;
	simple_lock_init(&swap_data_lock, "swap_data_lock");

	if (bdevvp(swapdev, &swapdev_vp)) {
		panic("vm_swap_init: can't get vnode for swap device");
	}

	/*
	 * create swap block resource map to map /dev/drum.   the range
	 * from 1 to INT_MAX allows 2 gigablocks of swap space.  note
	 * that block 0 is reserved (used to indicate an allocation
	 * failure, or no allocation).
	 */
	swapextent = extent_create("swapextent", 1, INT_MAX, M_SWAPMAP, 0, 0, EX_NOWAIT);
	if (swapextent == 0) {
		panic("swapinit: extent_create failed");
	}
}

struct vndxfer *
vndxfer_alloc(void)
{
	struct vndxfer *vndx;

	vndx = (struct vndxfer *)rmalloc(swapmap, sizeof(struct vndxfer *));
	return (vndx);
}

void
vndxfer_free(vndx)
	struct vndxfer *vndx;
{
	rmfree(swapmap, sizeof(vndx), vndx);
}

struct vndbuf *
vndbuf_alloc(void)
{
	struct vndbuf *vndb;

	vndb = (struct vndbuf *)rmalloc(swapmap, sizeof(struct vndbuf *));
	return (vndb);
}

void
vndbuf_free(vndb)
	struct vndbuf *vndb;
{
	rmfree(swapmap, sizeof(vndb), vndb);
}

struct swapbuf *
swapbuf_alloc(void)
{
	struct swapbuf *swbuf;

	swbuf = (struct swapbuf *)rmalloc(swapmap, sizeof(struct swapbuf *));
	return (swbuf);
}

void
swapbuf_free(swbuf)
	struct swapbuf *swbuf;
{
	rmfree(swapmap, sizeof(swbuf), swbuf);
}

void
vm_swap_stats(cmd, swp, sec, retval)
	int cmd;
	struct swdevt *swp;
	int sec;
	register_t *retval;
{
	struct swappri *spp;
	struct swapdev *sdp;
	int count = 0;

	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		for (sdp = CIRCLEQ_FIRST(&spp->spi_swapdev);
				sdp != (void*) &spp->spi_swapdev && sec-- > 0; sdp = CIRCLEQ_NEXT(sdp, swd_next)) {
			sdp->swd_swdevt->sw_inuse = btodb((u_int64_t)sdp->swd_npginuse << PAGE_SHIFT);
			bcopy(&sdp->swd_swdevt, swp, sizeof(struct swdevt));
			bcopy(sdp->swd_path, &swp->sw_path, sdp->swd_pathlen);
			count++;
			swp++;
		}
	}
	*retval = count;
	return;
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
		rmfree(swapmap, sizeof(*newspp), newspp);
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
	bool_t remove;
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
		rmfree(swapmap, sizeof(*spp), (memaddr_t)spp);
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

	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		CIRCLEQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp != (void *)&spp->spi_swapdev) {
				if (pgno >= sdp->swd_drumoffset && pgno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
					return (sdp);
				}
			}
		}
	}
	return (NULL);
}

int
swapctl()
{
	register struct swapctl_args {
		syscallarg(int) 	cmd;
		syscallarg(void *) 	arg;
		syscallarg(int) 	misc;
	} *uap = (struct swapctl_args *)u.u_ap;
	struct proc 	*p;
	int 			*retval;
	struct vnode 	*vp;
	struct nameidata nd;
	struct swappri *spp;
	struct swapdev *sdp;
	struct swdevt *swp;
	char userpath[PATH_MAX + 1];
	size_t	len;
	int	error, misc;
	int	priority;

	p = u.u_procp;

	misc = SCARG(uap, misc);

	/*
	 * ensure serialized syscall access by grabbing the swap_syscall_lock
	 */
	lockmgr(&swap_syscall_lock, LK_EXCLUSIVE, (void *)0, p->p_pid);

	/*
	 * we handle the non-priv NSWAP and STATS request first.
	 *
	 * SWAP_NSWAP: return number of config'd swap devices
	 * [can also be obtained with uvmexp sysctl]
	 */
	if (SCARG(uap, cmd) == SWAP_NSWAP) {
		*retval = cnt.v_nswapdev;
		error = 0;
		goto out;
	}

	/*
	 * all other requests require superuser privs.   verify.
	 */
	if ((error = suser1(p->p_ucred, &p->p_acflag))) {
		goto out;
	}

	/*
	 * at this point we expect a path name in arg.   we will
	 * use namei() to gain a vnode reference (vref), and lock
	 * the vnode (VOP_LOCK).
	 *
	 * XXX: a NULL arg means use the root vnode pointer (e.g. for
	 * miniroot)
	 */
	if (SCARG(uap, arg) == NULL) {
		vp = rootvp;		/* miniroot */
		if (vget(vp, LK_EXCLUSIVE)) {
			error = EBUSY;
			goto out;
		}
		if (SCARG(uap, cmd) == SWAP_ON &&
		    copystr("miniroot", userpath, sizeof userpath, &len))
			panic("swapctl: miniroot copy failed");
	} else {
		int		space;
		char	*where;

		if (SCARG(uap, cmd) == SWAP_ON) {
			if ((error = copyinstr(SCARG(uap, arg), userpath, sizeof userpath, &len)))
				goto out;
			space = UIO_SYSSPACE;
			where = userpath;
		} else {
			space = UIO_USERSPACE;
			where = (char *)SCARG(uap, arg);
		}
		NDINIT(&nd, LOOKUP, FOLLOW|LOCKLEAF, space, where, p);
		if ((error = namei(&nd))) {
			goto out;
		}
		vp = nd.ni_vp;
	}
	/* note: "vp" is referenced and locked */

	error = 0;		/* assume no error */
	switch(SCARG(uap, cmd)) {

	case SWAP_DUMPDEV:
		if (vp->v_type != VBLK) {
			error = ENOTBLK;
			break;
		}
		dumpdev = vp->v_rdev;
		break;

	case SWAP_CTL:
		/*
		 * get new priority, remove old entry (if any) and then
		 * reinsert it in the correct place.  finally, prune out
		 * any empty priority structures.
		 */
		priority = SCARG(uap, misc);
		spp = malloc(sizeof *spp, M_SWAPMAP, M_WAITOK);
		simple_lock(&swap_data_lock);
		if ((sdp = swaplist_find(vp, 1)) == NULL) {
			error = ENOENT;
		} else {
			swaplist_insert(sdp, spp, priority);
			swaplist_trim();
		}
		simple_unlock(&swap_data_lock);
		if (error)
			free(spp, M_SWAPMAP);
		break;

	case SWAP_ON:

		/*
		 * check for duplicates.   if none found, then insert a
		 * dummy entry on the list to prevent someone else from
		 * trying to enable this device while we are working on
		 * it.
		 */

		priority = SCARG(uap, misc);
		sdp = (struct swapdev *)malloc(sizeof *sdp, M_SWAPMAP, M_WAITOK);
		spp = (struct swappri *)malloc(sizeof *spp, M_SWAPMAP, M_WAITOK);
		bzero(sdp, sizeof(*sdp));
		swp->sw_flags = SW_FAKE;
		swp->sw_vp = vp;
		swp->sw_dev = (vp->v_type == VBLK) ? vp->v_rdev : NODEV;
		bufq_alloc(&sdp->swd_tab, BUFQ_DISKSORT|BUFQ_SORT_RAWBLOCK);
		simple_lock(&swap_data_lock);
		if (swaplist_find(vp, 0) != NULL) {
			error = EBUSY;
			simple_unlock(&swap_data_lock);
			bufq_free(&sdp->swd_tab);
			free(sdp, M_SWAPMAP);
			free(spp, M_SWAPMAP);
			break;
		}
		swaplist_insert(sdp, spp, priority);
		simple_unlock(&swap_data_lock);

		sdp->swd_pathlen = len;
		sdp->swd_path = malloc(sdp->swd_pathlen, M_SWAPMAP, M_WAITOK);
		if (copystr(userpath, sdp->swd_path, sdp->swd_pathlen, 0) != 0)
			panic("swapctl: copystr");

		/*
		 * we've now got a FAKE placeholder in the swap list.
		 * now attempt to enable swap on it.  if we fail, undo
		 * what we've done and kill the fake entry we just inserted.
		 * if swap_on is a success, it will clear the SWF_FAKE flag
		 */

		if ((error = swapdrum_on(p, swp)) != 0) {
			simple_lock(&swap_data_lock);
			(void) swaplist_find(vp, 1);  /* kill fake entry */
			swaplist_trim();
			simple_unlock(&swap_data_lock);
			bufq_free(&sdp->swd_tab);
			free(sdp->swd_path, M_SWAPMAP);
			free(sdp, M_SWAPMAP);
			break;
		}
		break;

	case SWAP_OFF:
		simple_lock(&swap_data_lock);
		if ((sdp = swaplist_find(vp, 0)) == NULL) {
			simple_unlock(&swap_data_lock);
			error = ENXIO;
			break;
		}

		/*
		 * If a device isn't in use or enabled, we
		 * can't stop swapping from it (again).
		 */
		if ((swp->sw_flags & (SW_INUSE|SW_ENABLE)) == 0) {
			simple_unlock(&swap_data_lock);
			error = EBUSY;
			break;
		}

		/*
		 * do the real work.
		 */
		error = swapdrum_off(p, swp);
		break;

	default:
		error = EINVAL;
	}

	/*
	 * done!  release the ref gained by namei() and unlock.
	 */
	vput(vp);

out:
	lockmgr(&swap_syscall_lock, LK_RELEASE, NULL);
	return (error);
}

int
swapdrum_on(p, swp)
	struct proc *p;
	struct swdevt *swp;
{
	struct swapdev *sdp;
	static int count = 0;	/* static */
	struct vnode *vp;
	const struct bdevsw *bdev;
	int error, npages, nblks, size;
	long addr;
	u_long result;
	struct vattr va;
	dev_t dev;
	char *name;

	/*
	 * we want to enable swapping on sdp. the swd_vp contains
	 * the vnode we want (locked and ref'd), and the swd_dev
	 * contains the dev_t of the file, if it a block device.
	 */
	vp = swp->sw_vp;
	dev = swp->sw_dev;
	sdp = swp->sw_swapdev;
	bdev = bdevsw_lookup(dev);

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
		if (bdev->d_psize == 0 || (nblks = (*bdev->d_psize)(dev)) == -1) {
			(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
			return (ENXIO);
		}
		break;
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
	default:
		(void) VOP_CLOSE(vp, FREAD | FWRITE, p->p_ucred, p);
		return (ENXIO);
	}

	/*
	 * save nblocks in a safe place and convert to pages.
	 */
	swp->sw_nblks = nblks;
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
	name = rmalloc(swapmap, 12);
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
	error = swap_miniroot(p, sdp, vp, npages, addr);
	if(error != 0) {
		return (error);
	}

	/*
	 * add anon's to reflect the swap space we added
	 */
	vm_anon_add(size);

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
	swp->sw_flags &= ~SW_FAKE;	/* going live */
	swp->sw_flags |= (SW_INUSE|SW_ENABLE);
	cnt.v_swpages += size;
	cnt.v_swpgavail += size;
	simple_unlock(&swap_data_lock);

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
swapdrum_off(p, swp)
	struct proc 	*p;
	struct swdevt 	*swp;
{
	struct swapdev 	*sdp;
	char			*name;
	int 			npages;

	sdp = swp->sw_swapdev;
	name = sdp->swd_ex->ex_name;
	npages = sdp->swd_npages;

	/* turn off the enable flag */
	swp->sw_flags &= ~SW_ENABLE;
	cnt.v_swpgavail -= npages;
	simple_unlock(&swap_data_lock);

	/*
	 * done with the vnode.
	 * drop our ref on the vnode before calling VOP_CLOSE()
	 * so that spec_close() can tell if this is the last close.
	 */
	vrele(swp->sw_vp);
	if(swp->sw_vp != rootvp) {
		(void) VOP_CLOSE(swp->sw_vp, FREAD|FWRITE, p->p_ucred, p);
	}

	simple_lock(&swap_data_lock);
	cnt.v_swpages -= npages;
	cnt.v_swpginuse -= sdp->swd_npgbad;
	if (swaplist_find(swp->sw_vp, 1) == NULL) {
		panic("swap_off: swapdev not in list");
	}
	swaplist_trim();
	simple_unlock(&swap_data_lock);

	/*
	 * free all resources!
	 */
	extent_free(swapextent, sdp->swd_drumoffset, sdp->swd_drumsize, EX_WAITOK);
	extent_destroy(sdp->swd_ex);
	rmfree(swapmap, sizeof(name), (memaddr_t)name);
	rmfree(swapmap, sizeof(sdp->swd_ex), (memaddr_t)sdp->swd_ex);
	bufq_free(&sdp->swd_tab);
	rmfree(swapmap, sizeof(sdp), (memaddr_t)sdp);

	return (0);
}

int
swap_miniroot(p, sdp, vp, npages, addr)
	struct proc 	*p;
	struct swapdev 	*sdp;
	struct vnode 	*vp;
	int npages;
	long addr;
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

/*
 * swstrategy: perform I/O on the drum
 *
 * => we must map the i/o request from the drum to the correct swapdev.
 */
void
swapdrum_strategy(swp, bp, vp)
	struct swdevt	*swp;
	struct buf 		*bp;
	struct vnode 	*vp;
{
	struct swapdev 	*sdp;
	int s, pageno, bn;

	/*
	 * convert block number to swapdev.   note that swapdev can't
	 * be yanked out from under us because we are holding resources
	 * in it (i.e. the blocks we are doing I/O on).
	 */
	pageno = dbtob((int64_t)bp->b_blkno) >> PAGE_SHIFT;
	simple_lock(&swap_data_lock);
	sdp = swapdrum_getsdp(pageno);
	simple_unlock(&swap_data_lock);
	if (sdp == NULL) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	if (sdp->swd_swdevt != swp) {
		sdp->swd_swdevt = swp;
	}
	if (swp->sw_swapdev != sdp) {
		swp->sw_swapdev = sdp;
	}

	/*
	 * convert drum page number to block number on this swapdev.
	 */

	pageno -= sdp->swd_drumoffset;					/* page # on swapdev */
	bn = btodb((u_int64_t)pageno << PAGE_SHIFT); 	/* convert to diskblock */

	/*
	 * for block devices we finish up here.
	 * for regular files we have to do more work which we delegate
	 * to sw_reg_strategy().
	 */

	switch (swp->sw_vp->v_type) {
	default:
		panic("swstrategy: vnode type 0x%x", swp->sw_vp->v_type);
		break;

	case VBLK:

		/*
		 * must convert "bp" from an I/O on /dev/drum to an I/O
		 * on the swapdev (sdp).
		 */
		s = splbio();
		bp->b_blkno = bn;			/* swapdev block number */
		bp->b_dev = swp->sw_dev;	/* swapdev dev_t */

		/*
		 * if we are doing a write, we have to redirect the i/o on
		 * drum's v_numoutput counter to the swapdevs.
		 */
		if ((bp->b_flags & B_READ) == 0) {
			if (vp == bp->b_vp) {
				vp->v_numoutput--;
				if ((vp->v_flag & VBWAIT) && vp->v_numoutput <= 0) {
					vp->v_flag &= ~VBWAIT;
				}
			}
			vwakeup(bp);				/* kills one 'v_numoutput' on drum */
			swp->sw_vp->v_numoutput++;	/* put it on swapdev */
		}
		if (bp->b_vp != NULL) {
			brelvp(bp);
		}

		/*
		 * finally plug in swapdev vnode and start I/O
		 */
		bp->b_vp = swp->sw_vp;
		splx(s);
		VOP_STRATEGY(bp);
		return;

	case VREG:
		/*
		 * delegate to sw_reg_strategy function.
		 */
		sw_reg_strategy(swp, bp, bn);
		return;
	}
	/* NOTREACHED */
}

/*
 * sw_reg_strategy: handle swap i/o to regular files
 */
static void
sw_reg_strategy(swp, bp, bn)
	struct swdevt	*swp;
	struct buf		*bp;
	int				bn;
{
	struct swapdev	*sdp;
	struct vnode	*vp;
	struct vndxfer	*vnx;
	daddr_t			nbn;
	caddr_t			addr;
	off_t			byteoff;
	int	s, off, nra, error, sz, resid;

	sdp = swp->sw_swapdev;

	/*
	 * allocate a vndxfer head for this transfer and point it to
	 * our buffer.
	 */
	vnx = vndxfer_alloc();
	vnx->vx_flags = VX_BUSY;
	vnx->vx_error = 0;
	vnx->vx_pending = 0;
	vnx->vx_bp = bp;
	vnx->vx_sdp = sdp;

	/*
	 * setup for main loop where we read filesystem blocks into
	 * our buffer.
	 */
	error = 0;
	bp->b_resid = bp->b_bcount;	/* nothing transfered yet! */
	addr = bp->b_data;			/* current position in buffer */
	byteoff = dbtob((u_int64_t)bn);

	for (resid = bp->b_resid; resid; resid -= sz) {
		struct vndbuf	*nbp;

		/*
		 * translate byteoffset into block number.  return values:
		 *   vp = vnode of underlying device
		 *  nbn = new block number (on underlying vnode dev)
		 *  nra = num blocks we can read-ahead (excludes requested
		 *	block)
		 */
		nra = 0;
		error = VOP_BMAP(swp->sw_vp, (byteoff / sdp->swd_bsize), &vp, &nbn, &nra);
		if (error == 0 && nbn == (daddr_t)-1) {
			/*
			 * this used to just set error, but that doesn't
			 * do the right thing.  Instead, it causes random
			 * memory errors.  The panic() should remain until
			 * this condition doesn't destabilize the system.
			 */
#if 1
			panic("sw_reg_strategy: swap to sparse file");
#else
			error = EIO;	/* failure */
#endif
		}

		/*
		 * punt if there was an error or a hole in the file.
		 * we must wait for any i/o ops we have already started
		 * to finish before returning.
		 *
		 * XXX we could deal with holes here but it would be
		 * a hassle (in the write case).
		 */
		if (error) {
			s = splbio();
			vnx->vx_error = error;	/* pass error up */
			goto out;
		}

		/*
		 * compute the size ("sz") of this transfer (in bytes).
		 */
		off = byteoff % sdp->swd_bsize;
		sz = (1 + nra) * sdp->swd_bsize - off;
		if (sz > resid)
			sz = resid;

		/*
		 * now get a buf structure.   note that the vb_buf is
		 * at the front of the nbp structure so that you can
		 * cast pointers between the two structure easily.
		 */
		nbp = vndbuf_alloc();
		nbp->vb_buf = &swbuf; /* XXX: FIX */
		nbp->vb_buf.b_flags    = bp->b_flags | B_CALL;
		nbp->vb_buf.b_bcount   = sz;
		nbp->vb_buf.b_bufsize  = sz;
		nbp->vb_buf.b_error    = 0;
		nbp->vb_buf.b_data     = addr;
		nbp->vb_buf.b_lblkno   = 0;
		nbp->vb_buf.b_blkno    = nbn + btodb(off);
		nbp->vb_buf.b_pblkno   = nbp->vb_buf.b_blkno;
		nbp->vb_buf.b_iodone   = sw_reg_iodone;
		nbp->vb_buf.b_vp       = vp;
		if (vp->v_type == VBLK) {
			nbp->vb_buf.b_dev = vp->v_rdev;
		}

		nbp->vb_xfer = vnx;	/* patch it back in to vnx */

		/*
		 * Just sort by block number
		 */
		s = splbio();
		if (vnx->vx_error != 0) {
			vndbuf_free(nbp);
			goto out;
		}
		vnx->vx_pending++;

		/* sort it in and start I/O if we are not over our limit */
		BUFQ_PUT(&sdp->swd_tab, &nbp->vb_buf);
		sw_reg_start(swp);
		splx(s);

		/*
		 * advance to the next I/O
		 */
		byteoff += sz;
		addr += sz;
	}

	s = splbio();

out: /* Arrive here at splbio */
	vnx->vx_flags &= ~VX_BUSY;
	if (vnx->vx_pending == 0) {
		if (vnx->vx_error != 0) {
			bp->b_error = vnx->vx_error;
			bp->b_flags |= B_ERROR;
		}
		vndxfer_free(vnx);
		biodone(bp);
	}
	splx(s);
}

/*
 * sw_reg_start: start an I/O request on the requested swapdev
 *
 * => reqs are sorted by b_rawblkno (above)
 */
static void
sw_reg_start(swp)
	struct swdevt	*swp;
{
	struct swapdev	*sdp;
	struct buf		*bp;
	struct vnode	*vp;

	sdp = swp->sw_swapdev;

	/* recursion control */
	if ((swp->sw_flags & SW_BUSY) != 0) {
		return;
	}

	swp->sw_flags |= SW_BUSY;

	while (sdp->swd_active < sdp->swd_maxactive) {
		bp = BUFQ_GET(&sdp->swd_tab);
		if (bp == NULL) {
			break;
		}
		sdp->swd_active++;
		vp = bp->b_vp;
		if ((bp->b_flags & B_READ) == 0) {
			simple_lock(&vp->v_interlock);
			vp->v_numoutput++;
			simple_unlock(&vp->v_interlock);
		}

		VOP_STRATEGY(bp);
	}
	swp->sw_flags &= ~SW_BUSY;
}

/*
 * sw_reg_iodone: one of our i/o's has completed and needs post-i/o cleanup
 *
 * => note that we can recover the vndbuf struct by casting the buf ptr
 */
static void
sw_reg_iodone(bp)
	struct buf *bp;
{
	struct vndbuf *vbp = (struct vndbuf *) bp;
	struct vndxfer *vnx = vbp->vb_xfer;
	struct buf *pbp = vnx->vx_bp;		/* parent buffer */
	struct swapdev	*sdp = vnx->vx_sdp;
	int s, resid, error;

	/*
	 * protect vbp at splbio and update.
	 */

	s = splbio();
	resid = vbp->vb_buf.b_bcount - vbp->vb_buf.b_resid;
	pbp->b_resid -= resid;
	vnx->vx_pending--;

	if (vbp->vb_buf.b_flags & B_ERROR) {
		/* pass error upward */
		error = vbp->vb_buf.b_error ? vbp->vb_buf.b_error : EIO;
		vnx->vx_error = error;
	}

	/*
	 * kill vbp structure
	 */
	vndbuf_free(vbp);

	/*
	 * wrap up this transaction if it has run to completion or, in
	 * case of an error, when all auxiliary buffers have returned.
	 */
	if (vnx->vx_error != 0) {
		/* pass error upward */
		pbp->b_flags |= B_ERROR;
		pbp->b_error = vnx->vx_error;
		if ((vnx->vx_flags & VX_BUSY) == 0 && vnx->vx_pending == 0) {
			vndxfer_free(vnx);
			biodone(pbp);
		}
	} else if (pbp->b_resid == 0) {
		KASSERT(vnx->vx_pending == 0);
		if ((vnx->vx_flags & VX_BUSY) == 0) {
			vndxfer_free(vnx);
			biodone(pbp);
		}
	}

	/*
	 * done!   start next swapdev I/O if one is pending
	 */
	sdp->swd_active--;
	sw_reg_start(sdp);
	splx(s);
}

/*
 * vm_swap_alloc: allocate space on swap
 *
 * => allocation is done "round robin" down the priority list, as we
 *	allocate in a priority we "rotate" the circle queue.
 * => space can be freed with vm_swap_free
 * => we return the page slot number in /dev/drum (0 == invalid slot)
 * => we lock swap_data_lock
 * => XXXMRG: "LESSOK" INTERFACE NEEDED TO EXTENT SYSTEM
 */
int
vm_swap_alloc(swp, nslots, lessok)
	struct swdevt	*swp;
	int 			*nslots;	/* IN/OUT */
	bool_t 			lessok;
{
	struct swapdev 	*sdp;
	struct swappri 	*spp;
	u_long			result;

	sdp = swp->sw_swapdev;

	/*
	 * no swap devices configured yet?   definite failure.
	 */
	if (cnt.v_nswapdev < 1) {
		return 0;
	}

	/*
	 * lock data lock, convert slots into blocks, and enter loop
	 */
	simple_lock(&swap_data_lock);

ReTry: /* XXXMRG */
	for (spp = LIST_FIRST(&swap_priority); spp != NULL; spp = LIST_NEXT(spp, spi_swappri)) {
		for (sdp = CIRCLEQ_FIRST(&spp->spi_swapdev); sdp != (void*) &spp->spi_swapdev; sdp = CIRCLEQ_NEXT(sdp, swd_next)) {
			/* if it's not enabled, then we can't swap from it */
			if ((swp->sw_flags & SW_ENABLE) == 0)
				continue;
			if (sdp->swd_npginuse + *nslots > sdp->swd_npages)
				continue;
			if (extent_alloc(sdp->swd_ex, *nslots, EX_NOALIGN, EX_NOBOUNDARY, EX_MALLOCOK | EX_NOWAIT, &result) != 0) {
				continue;
			}

			/*
			 * successful allocation!  now rotate the circleq.
			 */
			CIRCLEQ_REMOVE(&spp->spi_swapdev, sdp, swd_next);
			CIRCLEQ_INSERT_TAIL(&spp->spi_swapdev, sdp, swd_next);
			swp->sw_swapdev = sdp;
			sdp->swd_npginuse += *nslots;
			cnt.v_swpginuse += *nslots;
			simple_unlock(&swap_data_lock);
			/* done!  return drum slot number */
			return (result + sdp->swd_drumoffset);
		}
	}

	if (*nslots > 1 && lessok) {
		*nslots = 1;
		goto ReTry;
		/* XXXMRG: ugh!  extent should support this for us */
	}

	simple_unlock(&swap_data_lock);
	return (0);		/* failed */
}

/*
 * uvm_swap_markbad: keep track of swap ranges where we've had i/o errors
 *
 * => we lock uvm.swap_data_lock
 */
void
vm_swap_markbad(startslot, nslots)
	int startslot;
	int nslots;
{
	struct swapdev *sdp;

	simple_unlock(&swap_data_lock);
	sdp = swapdrum_getsdp(startslot);
	KASSERT(sdp != NULL);

	/*
	 * we just keep track of how many pages have been marked bad
	 * in this device, to make everything add up in swap_off().
	 * we assume here that the range of slots will all be within
	 * one swap device.
	 */

	KASSERT(cnt.v_swpgonly >= nslots);
	cnt.v_swpgonly -= nslots;
	sdp->swd_npgbad += nslots;
	simple_unlock(&swap_data_lock);
}

/*
 * vm_swap_free: free swap slots
 *
 * => this can be all or part of an allocation made by vm_swap_alloc
 * => we lock swap_data_lock
 */
void
vm_swap_free(startslot, nslots)
	int startslot;
	int nslots;
{
	struct swapdev *sdp;

	/*
	 * convert drum slot offset back to sdp, free the blocks
	 * in the extent, and return.   must hold pri lock to do
	 * lookup and access the extent.
	 */
	simple_lock(&swap_data_lock);
	sdp = swapdrum_getsdp(startslot);

#ifdef DIAGNOSTIC
	if (cnt.v_nswapdev < 1)
		panic("vm_swap_free: cnt.v_nswapdev < 1\n");
	if (sdp == NULL) {
		printf("vm_swap_free: startslot %d, nslots %d\n", startslot, nslots);
		panic("vm_swap_free: unmapped address\n");
	}
#endif

	if (extent_free(sdp->swd_ex, startslot - sdp->swd_drumoffset, nslots, EX_MALLOCOK|EX_NOWAIT) != 0) {
		printf("warning: resource shortage: %d slots of swap lost\n", nslots);
	}

	sdp->swd_npginuse -= nslots;
	cnt.v_swpginuse -= nslots;
#ifdef DIAGNOSTIC
	if (sdp->swd_npginuse < 0)
		panic("vm_swap_free: inuse < 0");
#endif
	simple_unlock(&swap_data_lock);
}

void
vm_swapbuf_init(bp, p)
	struct buf  	*bp;
	struct proc 	*p;
{
	register struct swapbuf *sbp;
	int i;

	sbp = swapbuf_alloc();
	TAILQ_INIT(&sbp->sw_bswlist);
	sbp->sw_buf = bp;
	sbp->sw_nswbuf = nswbuf;

	for (i = 0; i < nswbuf - 1; i++, sbp->sw_buf++) {
		TAILQ_INSERT_HEAD(&sbp->sw_bswlist, sbp->sw_buf, b_freelist);
		sbp->sw_buf->b_rcred = sbp->sw_buf->b_wcred = p->p_ucred;
		LIST_NEXT(sbp->sw_buf, b_vnbufs) = NOLIST;
	}
	sbp->sw_buf->b_rcred = sbp->sw_buf->b_wcred = p->p_ucred;
	LIST_NEXT(sbp->sw_buf, b_vnbufs) = NOLIST;
	TAILQ_REMOVE(&sbp->sw_bswlist, sbp->sw_buf, b_actq);
}

struct buf *
vm_getswapbuf(sbp)
	struct swapbuf *sbp;
{
	struct buf *bp;
	int s;

	if (sbp == NULL) {
		sbp = swapbuf_alloc();
	}
	s = splbio();
	while((bp = TAILQ_FIRST(&sbp->sw_bswlist)) == NULL) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)&bp, BPRIO_DEFAULT);
	}
	TAILQ_REMOVE(&sbp->sw_bswlist, bp, b_freelist);
	splx(s);

	if (bp == NULL) {
		bp = (struct buf *)malloc(sizeof(*bp), M_TEMP, M_WAITOK);
	}
	bzero(bp, sizeof(*bp));
	BUF_INIT(bp);

	bp->b_rcred = bp->b_wcred = NOCRED;
	LIST_NEXT(bp, b_vnbufs) = NOLIST;
	sbp->sw_buf = bp;
	bp->b_swbuf = sbp;
	return (bp);
}

void
vm_putswapbuf(sbp, bp)
	struct swapbuf 	*sbp;
	struct buf 		*bp;
{
	int s;

	if(sbp == NULL) {
		sbp = vm_swapbuf_get(bp);
	}
	s = splbio();
	if (bp->b_vp) {
		brelvp(bp);
	}
	if (bp->b_flags & B_WANTED) {
		bp->b_flags &= ~B_WANTED;
		wakeup((caddr_t) bp);
	}
	sbp->sw_buf = bp;
	//bp->b_swbuf = sbp;
	TAILQ_INSERT_HEAD(&sbp->sw_bswlist, bp, b_freelist);
	free(bp, M_TEMP);
	splx(s);
}

struct swapbuf *
vm_swapbuf_get(bp)
	struct buf 	*bp;
{
	register struct swapbuf *sbp;

	if (bp->b_swbuf == NULL) {
		bp->b_swbuf = swapbuf_alloc();
	}
	sbp = bp->b_swbuf;
	return (sbp);
}
