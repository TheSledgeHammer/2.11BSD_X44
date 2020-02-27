/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * Copyright 2015 Toomas Soome <tsoome@me.com>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
/*__FBSDID("$FreeBSD$"); */

/*
 * Simple hashed block cache
 */


#include "bootstrap2.h"

/* #define BCACHE_DEBUG */

#ifdef BCACHE_DEBUG
# define DPRINTF(fmt, args...)	printf("%s: " fmt "\n" , __func__ , ## args)
#else
# define DPRINTF(fmt, args...)	((void)0)
#endif

struct bcachectl
{
    daddr_t	bc_blkno;
    int		bc_count;
};

/*
 * bcache per device node. cache is allocated on device first open and freed
 * on last close, to save memory. The issue there is the size; biosdisk
 * supports up to 31 (0x1f) devices. Classic setup would use single disk
 * to boot from, but this has changed with zfs.
 */
struct bcache {
    struct bcachectl	*bcache_ctl;
    caddr_t				bcache_data;
    size_t				bcache_nblks;
    size_t				ra;
};

static u_int bcache_total_nblks;	/* set by bcache_init */
static u_int bcache_blksize;		/* set by bcache_init */
static u_int bcache_numdev;			/* set by bcache_add_dev */
/* statistics */
static u_int bcache_units;			/* number of devices with cache */
static u_int bcache_unit_nblks;		/* nblocks per unit */
static u_int bcache_hits;
static u_int bcache_misses;
static u_int bcache_ops;
static u_int bcache_bypasses;
static u_int bcache_bcount;
static u_int bcache_rablks;

#define	BHASH(bc, blkno)	((blkno) & ((bc)->bcache_nblks - 1))
#define	BCACHE_LOOKUP(bc, blkno)	\
	((bc)->bcache_ctl[BHASH((bc), (blkno))].bc_blkno != (blkno))
#define	BCACHE_READAHEAD	256
#define	BCACHE_MINREADAHEAD	32

static void	bcache_invalidate(struct bcache *bc, daddr_t blkno);
static void	bcache_insert(struct bcache *bc, daddr_t blkno);
static void	bcache_free_instance(struct bcache *bc);

/*
 * Initialise the cache for (nblks) of (bsize).
 */
void
bcache_init(size_t nblks, size_t bsize)
{
    /* set up control data */
    bcache_total_nblks = nblks;
    bcache_blksize = bsize;
}

/*
 * add number of devices to bcache. we have to divide cache space
 * between the devices, so bcache_add_dev() can be used to set up the
 * number. The issue is, we need to get the number before actual allocations.
 * bcache_add_dev() is supposed to be called from device init() call, so the
 * assumption is, devsw dv_init is called for plain devices first, and
 * for zfs, last.
 */
void
bcache_add_dev(int devices)
{
    bcache_numdev += devices;
}

void *
bcache_allocate(void)
{
	u_int i;
	struct bcache *bc = malloc(sizeof(struct bcache));
	int disks = bcache_numdev;

	if (disks == 0)
		disks = 1; /* safe guard */

	if (bc == NULL) {
		errno = ENOMEM;
		return (bc);
	}

	/*
	 * the bcache block count must be power of 2 for hash function
	 */
	i = fls(disks) - 1; 	/* highbit - 1 */
	if (disks > (1 << i)) 	/* next power of 2 */
		i++;

	bc->bcache_nblks = bcache_total_nblks >> i;
	bcache_unit_nblks = bc->bcache_nblks;
	bc->bcache_data = malloc(bc->bcache_nblks * bcache_blksize);
	if (bc->bcache_data == NULL) {
		/* dont error out yet. fall back to 32 blocks and try again */
		bc->bcache_nblks = 32;
		bc->bcache_data = malloc(
				bc->bcache_nblks * bcache_blksize + sizeof(uint32_t));
	}

	bc->bcache_ctl = malloc(bc->bcache_nblks * sizeof(struct bcachectl));

	if ((bc->bcache_data == NULL) || (bc->bcache_ctl == NULL)) {
		bcache_free_instance(bc);
		errno = ENOMEM;
		return (NULL);
	}

	/* Flush the cache */
	for (i = 0; i < bc->bcache_nblks; i++) {
		bc->bcache_ctl[i].bc_count = -1;
		bc->bcache_ctl[i].bc_blkno = -1;
	}
	bcache_units++;
	bc->ra = BCACHE_READAHEAD; /* optimistic read ahead */
	return (bc);
}

void
bcache_free(void *cache)
{
	struct bcache *bc = cache;

	if (bc == NULL)
		return;

	bcache_free_instance(bc);
	bcache_units--;
}
