/*	$NetBSD: uvm_swap.h,v 1.6 2002/03/18 11:43:01 manu Exp $	*/

/*
 * Copyright (c) 1997 Matthew R. Green
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
 * from: Id: uvm_swap.h,v 1.1.2.6 1997/12/15 05:39:31 mrg Exp
 */

#ifndef _UVM_UVM_SWAP_H_
#define _UVM_UVM_SWAP_H_

#include <devel/vm/include/swap_pager.h>

/*
 * swapdev: describes a single swap partition/file
 *
 * note the following should be true:
 * swd_inuse <= swd_nblks  [number of blocks in use is <= total blocks]
 * swd_nblks <= swd_mapsize [because mapsize includes miniroot+disklabel]
 */
struct swapdev {
	struct oswapent 					swd_ose;
#define	swd_dev							swd_ose.ose_dev			/* device id */
#define	swd_flags						swd_ose.ose_flags		/* flags:inuse/enable/fake */
#define	swd_priority					swd_ose.ose_priority	/* our priority */
	/* also: swd_ose.ose_nblks, swd_ose.ose_inuse */
	char								*swd_path;			/* saved pathname of device */
	int									swd_pathlen;		/* length of pathname */
	int									swd_npages;			/* #pages we can use */
	int									swd_npginuse;		/* #pages in use */
	int									swd_npgbad;			/* #pages bad */
	int									swd_drumoffset;		/* page0 offset in drum */
	int									swd_drumsize;		/* #pages in drum */
	struct extent						*swd_ex;			/* extent for this swapdev */
	char								swd_exname[12];		/* name of extent above */
	struct vnode						*swd_vp;			/* backing vnode */
	CIRCLEQ_ENTRY(swapdev)				swd_next;			/* priority circleq */

	int									swd_bsize;			/* blocksize (bytes) */
	int									swd_maxactive;		/* max active i/o reqs */
	struct buf_queue					swd_tab;			/* buffer list */
	int									swd_active;			/* number of active buffers */
};

/*
 * swap device priority entry; the list is kept sorted on `spi_priority'.
 */
struct swappri {
	int									spi_priority;     /* priority */
	CIRCLEQ_HEAD(spi_swapdev, swapdev)	spi_swapdev;
	/* circleq of swapdevs at this priority */
	LIST_ENTRY(swappri)					spi_swappri;      /* global list of pri's */
};

/*
 * The following two structures are used to keep track of data transfers
 * on swap devices associated with regular files.
 * NOTE: this code is more or less a copy of vnd.c; we use the same
 * structure names here to ease porting..
 */
struct vndxfer {
	struct buf							*vx_bp;		/* Pointer to parent buffer */
	struct swapdev						*vx_sdp;
	int									vx_error;
	int									vx_pending;	/* # of pending aux buffers */
	int									vx_flags;
#define VX_BUSY							1
#define VX_DEAD							2
};

struct vndbuf {
	struct buf							vb_buf;
	struct vndxfer						*vb_xfer;
};

#define	SWSLOT_BAD	(-1)

#include <sys/syslimits.h>

/* These structures are used to return swap information for userland */

/*
 * NetBSD 1.3 swapctl(SWAP_STATS, ...) swapent structure; now used as an
 * overlay for both the new swapent structure, and the hidden swapdev
 * structure (see sys/uvm/uvm_swap.c).
 */
struct oswapent {
	dev_t	ose_dev;		/* device id */
	int		ose_flags;		/* flags */
	int		ose_nblks;		/* total blocks */
	int		ose_inuse;		/* blocks in use */
	int		ose_priority;	/* priority of this device */
};

struct swapent {
	struct oswapent se_ose;
#define	se_dev		se_ose.ose_dev
#define	se_flags	se_ose.ose_flags
#define	se_nblks	se_ose.ose_nblks
#define	se_inuse	se_ose.ose_inuse
#define	se_priority	se_ose.ose_priority
	char			se_path[PATH_MAX+1];	/* path name */
};

#define SWAP_ON			1		/* begin swapping on device */
#define SWAP_OFF		2		/* stop swapping on device */
#define SWAP_NSWAP		3		/* how many swap devices ? */
#define SWAP_OSTATS		4		/* old SWAP_STATS, no se_path */
#define SWAP_CTL		5		/* change priority on device */
#define SWAP_STATS		6		/* get device info */
#define SWAP_DUMPDEV	7		/* use this device as dump device */
#define SWAP_GETDUMPDEV	8		/* use this device as dump device */

#define SWF_INUSE	0x00000001	/* in use: we have swapped here */
#define SWF_ENABLE	0x00000002	/* enabled: we can swap here */
#define SWF_BUSY	0x00000004	/* busy: I/O happening here */
#define SWF_FAKE	0x00000008	/* fake: still being built */

#ifdef _KERNEL

struct		swblock;
struct 		swpager;

int			vm_swap_get (struct vm_page *, int, int);
int			vm_swap_put (int, struct vm_page **, int, int);
int			vm_swap_alloc (int *, boolean_t);
void		vm_swap_free (int, int);
void		vm_swap_markbad (int, int);
void		vm_swap_stats (int, struct swapent *, int, register_t *);

#endif /* _KERNEL */

#endif /* _UVM_UVM_SWAP_H_ */
