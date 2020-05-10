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
 *
 * @(#)vfs_htree.c	1.00
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>

#include <devel/ufml/ext2fs_htree.h>
#include <devel/ufml/ext2fs_hash.h>

#define	KDASSERT(x) 		assert(x)
#define	KASSERT(x) 			assert(x)
#define	htree_alloc(s) 		malloc(s)
#define	htree_free(a, s) 	free(a)
#define	htree_calloc(n, s) 	calloc((n), (s))

#define htree_lock()		simplelock()
#define htree_unlock() 		simpleunlock()

struct htree_dealloc {
	TAILQ_ENTRY(htree_dealloc) 	hd_entries;
	daddr_t 					hd_blkno;	/* address of block */
	int 						hd_len;		/* size of block */
};

TAILQ_HEAD(htchain, htree_chain);
struct htree_chain {
	struct htchain				hc_head;
	TAILQ_ENTRY(htree_chain) 	hc_entry;
	struct vnode 				*hc_devvp;
	struct mount 				*hc_mount;

	int 						hc_dkcache;
};

static struct htree_dealloc *htree_dealloc;

void
htree_init()
{
	&htree_dealloc = (struct htree_dealloc *) malloc(sizeof(struct htree_dealloc), M_HTREE, NULL);
}

void
htree_fini()
{
	FREE(&htree_dealloc, M_HTREE);
}

static void
htree_dkcache_init(hc)
	struct htree_chain *hc;
{
	int error;

	/* Get disk cache flags */
	error = VOP_IOCTL(hc->hc_devvp, DIOCGCACHE, &hc->hc_dkcache, FWRITE, FSCRED);
	if (error) {
		/* behave as if there was a write cache */
		hc->hc_dkcache = DKCACHE_WRITE;
	}
}

int
htree_start()
{

}

int
htree_stop()
{

}

int
htree_read()
{

}

int
htree_write()
{

}

