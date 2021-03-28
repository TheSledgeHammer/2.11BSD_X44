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

#ifndef _VM_SWAP_H_
#define _VM_SWAP_H_

#include <sys/conf.h>
#include <sys/queue.h>

/*
 * Swap device switch (WIP)
 */
struct swapdevsw {
	int 		(*sw_allocate)();
	int 		(*sw_free)();
	int			(*sw_create)(struct swapdev *swdev);
	int			(*sw_destroy)(struct swapdev *swdev);
	int 		(*sw_read)(dev_t dev, struct uio *uio, int ioflag);
	int 		(*sw_write)(dev_t dev, struct uio *uio, int ioflag);
	void 		(*sw_strategy)(struct buf *bp);
};

#define	SWSLOT_BAD	(-1)

/*
 * swapdev: describes a single swap partition/file
 *
 * note the following should be true:
 * swd_inuse <= swd_nblks  [number of blocks in use is <= total blocks]
 * swd_nblks <= swd_mapsize [because mapsize includes miniroot+disklabel]
 */
struct swapdev {
	struct swdevt						*swd_swdevt;	/* Swap device table */
	int									swd_priority;	/* our priority */
	int									swd_nblks;		/* blocks in this device */
	char								*swd_path;		/* saved pathname of device */
	int									swd_pathlen;	/* length of pathname */
	int									swd_npages;		/* #pages we can use */
	int									swd_npginuse;	/* #pages in use */
	int									swd_npgbad;		/* #pages bad */
	int									swd_drumoffset;	/* page0 offset in drum */
	int									swd_drumsize;	/* #pages in drum */
	struct extent						*swd_ex;		/* extent for this swapdev */
	char								swd_exname[12];	/* name of extent above */
	TAILQ_ENTRY(swapdev)				swd_next;		/* priority tailq */

	int									swd_bsize;		/* blocksize (bytes) */
	int									swd_maxactive;	/* max active i/o reqs */
	int									swd_active;		/* # of active i/o reqs */
	struct bufq							swd_bufq;
	struct ucred						*swd_cred;		/* cred for file access */
};

/*
 * swap device priority entry; the list is kept sorted on `spi_priority'.
 */
struct swappri {
	int									spi_priority;   /* priority */
	TAILQ_HEAD(spi_swapdev, swapdev)	spi_swapdev; 	/* tailq of swapdevs at this priority */
	LIST_ENTRY(swappri)					spi_swappri;    /* global list of pri's */
};

/*
* The following two structures are used to keep track of data transfers
* on swap devices associated with regular files.
* NOTE: this code is more or less a copy of vnd.c; we use the same
* structure names here to ease porting..
*/
struct vndxfer {
	struct buf							*vx_bp;			/* Pointer to parent buffer */
	struct swapdev						*vx_sdp;
	int									vx_error;
	int									vx_pending;		/* # of pending aux buffers */
	int									vx_flags;
#define VX_BUSY							1
#define VX_DEAD							2
};

struct vndbuf {
	struct buf							vb_buf;
	struct vndxfer						*vb_xfer;
};

#endif /* SYS_DEVEL_VM_UVM_VM_SWAP_H_ */
