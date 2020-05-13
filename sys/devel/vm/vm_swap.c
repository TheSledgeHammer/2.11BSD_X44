/*	$NetBSD: uvm_swap.c,v 1.64.4.1 2002/10/02 01:14:42 lukem Exp $	*/

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

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: uvm_swap.c,v 1.64.4.1 2002/10/02 01:14:42 lukem Exp $"); */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/disklabel.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/extent.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <stdlib.h>
#include <stdio.h>

#include <devel/vm/include/vm.h>

#include <miscfs/specfs/specdev.h>

/*
 * uvm_swap.c: manage configuration and i/o to swap space.
 */

/*
 * swap space is managed in the following way:
 *
 * each swap partition or file is described by a "swapdev" structure.
 * each "swapdev" structure contains a "swapent" structure which contains
 * information that is passed up to the user (via system calls).
 *
 * each swap partition is assigned a "priority" (int) which controls
 * swap parition usage.
 *
 * the system maintains a global data structure describing all swap
 * partitions/files.   there is a sorted LIST of "swappri" structures
 * which describe "swapdev"'s at that priority.   this LIST is headed
 * by the "swap_priority" global var.    each "swappri" contains a
 * CIRCLEQ of "swapdev" structures at that priority.
 *
 * locking:
 *  - swap_syscall_lock (sleep lock): this lock serializes the swapctl
 *    system call and prevents the swap priority list from changing
 *    while we are in the middle of a system call (e.g. SWAP_STATS).
 *  - uvm.swap_data_lock (simple_lock): this lock protects all swap data
 *    structures including the priority list, the swapdev structures,
 *    and the swapmap extent.
 *
 * each swap device has the following info:
 *  - swap device in use (could be disabled, preventing future use)
 *  - swap enabled (allows new allocations on swap)
 *  - map info in /dev/drum
 *  - vnode pointer
 * for swap files only:
 *  - block size
 *  - max byte count in buffer
 *  - buffer
 *
 * userland controls and configures swap with the swapctl(2) system call.
 * the sys_swapctl performs the following operations:
 *  [1] SWAP_NSWAP: returns the number of swap devices currently configured
 *  [2] SWAP_STATS: given a pointer to an array of swapent structures
 *	(passed in via "arg") of a size passed in via "misc" ... we load
 *	the current swap config into the array. The actual work is done
 *	in the uvm_swap_stats(9) function.
 *  [3] SWAP_ON: given a pathname in arg (could be device or file) and a
 *	priority in "misc", start swapping on it.
 *  [4] SWAP_OFF: as SWAP_ON, but stops swapping to a device
 *  [5] SWAP_CTL: changes the priority of a swap device (new priority in
 *	"misc")
 */

/* /dev/drum */
bdev_decl(sw);
cdev_decl(sw);

/*
 * local variables
 */
static struct extent *swapmap;		/* controls the mapping of /dev/drum */
static struct vndxfer *vndxfer;
static struct vndbuf *vndbuf;

/* list of all active swap devices [by priority] */
LIST_HEAD(swap_priority, swappri);
static struct swap_priority swap_priority;

/* locks */
struct lock swap_syscall_lock;

/*
 * prototypes
 */
static struct swapdev	*swapdrum_getsdp (int);

static struct swapdev	*swaplist_find (struct vnode *, int);
static void	 			swaplist_insert (struct swapdev *, struct swappri *, int);
static void				swaplist_trim (void);

static int 				swap_on (struct proc *, struct swapdev *);
static int 				swap_off (struct proc *, struct swapdev *);

static void 			sw_reg_strategy (struct swapdev *, struct buf *, int);
static void 			sw_reg_iodone (struct buf *);
static void 			sw_reg_start (struct swapdev *);

static int 				vm_swap_io (struct vm_page **, int, int, int);

/*
 * uvm_swap_init: init the swap system data structures and locks
 *
 * => called at boot time from init_main.c after the filesystems
 *	are brought up (which happens after uvm_init())
 */
void
vm_swap_init()
{
	/*
	 * first, init the swap list, its counter, and its lock.
	 * then get a handle on the vnode for /dev/drum by using
	 * the its dev_t number ("swapdev", from MD conf.c).
	 */

	LIST_INIT(&swap_priority);
	uvmexp.nswapdev = 0;
	lockinit(&swap_syscall_lock, PVM, "swapsys", 0, 0);
	simple_lock_init(&vm.swap_data_lock);

	if (bdevvp(swapdev, &swapdev_vp))
		panic("uvm_swap_init: can't get vnode for swap device");

	/*
	 * create swap block resource map to map /dev/drum.   the range
	 * from 1 to INT_MAX allows 2 gigablocks of swap space.  note
	 * that block 0 is reserved (used to indicate an allocation
	 * failure, or no allocation).
	 */
	swapmap = extent_create("swapmap", 1, INT_MAX, M_VMSWAP, 0, 0, EX_NOWAIT);
	if (swapmap == 0)
		panic("uvm_swap_init: extent_create failed");

	/*
	 * allocate pools for structures used for swapping to files.
	 */
	MALLOC(&vndxfer, struct vndxfer, sizeof(struct vndxfer), M_WAITOK);
	MALLOC(&vndbuf, struct vndbuf, sizeof(struct vndbuf), M_WAITOK);
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
static void
swaplist_insert(sdp, newspp, priority)
	struct swapdev *sdp;
	struct swappri *newspp;
	int priority;
{
	struct swappri *spp, *pspp;
	//UVMHIST_FUNC("swaplist_insert"); UVMHIST_CALLED(pdhist);

	/*
	 * find entry at or after which to insert the new device.
	 */
	pspp = NULL;
	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		if (priority <= spp->spi_priority)
			break;
		pspp = spp;
	}

	/*
	 * new priority?
	 */
	if (spp == NULL || spp->spi_priority != priority) {
		spp = newspp;  /* use newspp! */
		UVMHIST_LOG(pdhist, "created new swappri = %d",
			    priority, 0, 0, 0);

		spp->spi_priority = priority;
		CIRCLEQ_INIT(&spp->spi_swapdev);

		if (pspp)
			LIST_INSERT_AFTER(pspp, spp, spi_swappri);
		else
			LIST_INSERT_HEAD(&swap_priority, spp, spi_swappri);
	} else {
	  	/* we don't need a new priority structure, free it */
		FREE(newspp, M_VMSWAP);
	}

	/*
	 * priority found (or created).   now insert on the priority's
	 * circleq list and bump the total number of swapdevs.
	 */
	sdp->swd_priority = priority;
	CIRCLEQ_INSERT_TAIL(&spp->spi_swapdev, sdp, swd_next);
	uvmexp.nswapdev++;
}

/*
 * swaplist_find: find and optionally remove a swap device from the
 *	global list.
 *
 * => caller must hold both swap_syscall_lock and uvm.swap_data_lock
 * => we return the swapdev we found (and removed)
 */
static struct swapdev *
swaplist_find(vp, remove)
	struct vnode *vp;
	boolean_t remove;
{
	struct swapdev *sdp;
	struct swappri *spp;

	/*
	 * search the lists for the requested vp
	 */

	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		CIRCLEQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp->swd_vp == vp) {
				if (remove) {
					CIRCLEQ_REMOVE(&spp->spi_swapdev,
					    sdp, swd_next);
					uvmexp.nswapdev--;
				}
				return(sdp);
			}
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
static void
swaplist_trim()
{
	struct swappri *spp, *nextspp;

	for (spp = LIST_FIRST(&swap_priority); spp != NULL; spp = nextspp) {
		nextspp = LIST_NEXT(spp, spi_swappri);
		if (CIRCLEQ_FIRST(&spp->spi_swapdev) !=
		    (void *)&spp->spi_swapdev)
			continue;
		LIST_REMOVE(spp, spi_swappri);
		free(spp, M_VMSWAP);
	}
}

/*
 * swapdrum_getsdp: given a page offset in /dev/drum, convert it back
 * to the "swapdev" that maps that section of the drum.
 *
 * => each swapdev takes one big contig chunk of the drum
 * => caller must hold uvm.swap_data_lock
 */
static struct swapdev *
swapdrum_getsdp(pgno)
	int pgno;
{
	struct swapdev *sdp;
	struct swappri *spp;

	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		CIRCLEQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp->swd_flags & SWF_FAKE)
				continue;
			if (pgno >= sdp->swd_drumoffset &&
			    pgno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
				return sdp;
			}
		}
	}
	return NULL;
}

