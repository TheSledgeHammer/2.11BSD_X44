/*
 * vfs_wapbl.c
 *
 *  Created on: 18 Feb 2020
 *      Author: marti
 */

#include <sys/param.h>
//#include <sys/bitops.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <devel/ufs_wapbl/wapbl.h>
#include <devel/ufs_wapbl/wapbl_replay.h>

#ifdef _KERNEL

//#include <sys/atomic.h>
#include <sys/conf.h>
#include <sys/malloc.h>
//#include <sys/evcnt.h>
#include <sys/file.h>
//#include <sys/kauth.h>
#include <sys/kernel.h>
//#include <sys/module.h>
#include <sys/mount.h>
//#include <sys/mutex.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/vnode.h>

#include <vfs/specfs/specdev.h>

#define	wapbl_alloc(s) kmem_alloc((s), KM_SLEEP)
#define	wapbl_free(a, s) kmem_free((a), (s))
#define	wapbl_calloc(n, s) kmem_zalloc((n)*(s), KM_SLEEP)

static struct sysctllog *wapbl_sysctl;
static int wapbl_flush_disk_cache = 1;
static int wapbl_verbose_commit = 0;
static int wapbl_allow_dpofua = 0; 	/* switched off by default for now */
static int wapbl_journal_iobufs = 4;

static inline size_t wapbl_space_free(size_t, off_t, off_t);

#else /* !_KERNEL */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	KDASSERT(x) assert(x)
#define	KASSERT(x) assert(x)
#define	wapbl_alloc(s) malloc(s)
#define	wapbl_free(a, s) free(a)
#define	wapbl_calloc(n, s) calloc((n), (s))

#endif /* !_KERNEL */

static struct wapbl_entry *wapbl_entry_pool;
static struct wapbl_dealloc *wapbl_dealloc_pool;

static void
wapbl_init(void)
{
	MALLOC(&wapbl_entry_pool, struct wapbl_entry, sizeof(struct wapbl_entry), M_WAPBL, NULL);
	MALLOC(&wapbl_dealloc_pool, struct wapbl_dealloc, sizeof(struct wapbl_dealloc), M_WAPBL, NULL);
}


static int
wapbl_fini(void)
{
	FREE(&wapbl_dealloc_pool, M_WAPBL);
	FREE(&wapbl_entry_pool, M_WAPBL);
}


static void
wapbl_dkcache_init(struct wapbl *wl)
{
	int error;

	/* Get disk cache flags */
	error = VOP_IOCTL(wl->wl_devvp, DIOCGCACHE, &wl->wl_dkcache,
	    FWRITE, FSCRED);
	if (error) {
		/* behave as if there was a write cache */
		wl->wl_dkcache = DKCACHE_WRITE;
	}

	/* Use FUA instead of cache flush if available */
	if (ISSET(wl->wl_dkcache, DKCACHE_FUA))
		wl->wl_jwrite_flags |= B_MEDIA_FUA;

	/* Use DPO for journal writes if available */
	if (ISSET(wl->wl_dkcache, DKCACHE_DPO))
		wl->wl_jwrite_flags |= B_MEDIA_DPO;
}
