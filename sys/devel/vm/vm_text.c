/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_text.c	1.2 (2.11BSD GTE) 11/26/94
 */

#include <sys/extent.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_text.h>

struct txtlist      vm_text_list;
simple_lock_data_t	vm_text_list_lock;
struct vm_xstats 	*xstats;			/* cache statistics */
int					xcache;				/* number of "sticky" texts retained */

/*
 * initialize text table
 */
void
vm_text_init(xp)
	vm_text_t 		xp;
{
    int ntexts;

    simple_lock_init(&vm_text_list_lock, "vm_text_list_lock");
    TAILQ_INIT(&vm_text_list);
}

/*
 * Attach to a shared text segment.  If there is no shared text, just
 * return.  If there is, hook up to it.  If it is not available from
 * core or swap, it has to be read in from the vnode (vp); the written
 * bit is set to force it to be written out as appropriate.  If it is
 * not available from core, a swap has to be done to get it back.
 */
void
vm_xalloc(vp, tsize, toff)
	register struct vnode *vp;
	u_long 	tsize;
	off_t 	toff;
{
	register vm_text_t xp;
	register struct proc *p;

	p = vp->v_proc;
	while ((xp == vp->v_text) != NULL) {
		if (xp->psx_flag & XLOCK) {
			xwait(xp);
			continue;
		}
		xlock(&vm_text_list_lock);
		if (TAILQ_LAST(&vm_text_list, txtlist)) {
			TAILQ_INSERT_HEAD(&vm_text_list, xp, psx_list);
			xstats->psxs_alloc_cachehit++;
			xp->psx_flag &= ~XUNUSED;
		} else {
			xstats->psxs_alloc_inuse++;
		}
		xp->psx_count++;
		p->p_textp = xp;
		if (!xp->psx_caddr && !xp->psx_ccount) {
			vm_xexpand(p, xp);
		} else {
			++xp->psx_ccount;
		}
		xunlock(&vm_text_list_lock);
		return;
	}
	xp = TAILQ_FIRST(&vm_text_list);
	if (xp == NULL) {
		psignal(p, SIGKILL);
		return;
	}
	TAILQ_INSERT_HEAD(&vm_text_list, xp, psx_list);
	if (xp->psx_vptr) {
		if (xp->psx_flag & XUNUSED) {
			xstats->psxs_alloc_unused++;
		}
		vm_xuntext(xp);
	}
	xp->psx_flag = XLOAD | XLOCK;
	if (p->p_flag & SPAGV) {
		xp->psx_flag |= XPAGV;
	}

	xp->psx_size = clrnd(btoc(tsize));
	/* 2.11BSD overlays were here */
	if((xp->psx_daddr = vm_vsxalloc(xp)) == NULL) {
		/* flush text cache and try again */
		if (vm_xpurge() == 0 || vm_vsxalloc(xp) == NULL) {
			swkill(p, "xalloc: no swap space");
			return;
		}
	}
	xp->psx_count = 1;
	xp->psx_ccount = 0;
	xp->psx_vptr = vp;
	vp->v_flag |= VTEXT;
	vp->v_text = xp;
	VREF(vp);
	p->p_textp = xp;

	(void) vn_rdwr(UIO_READ, vp, (caddr_t)ctob(tptov(p, 0)), tsize, toff, UIO_USERSPACE, (IO_UNIT|IO_NODELOCKED), p->p_cred, (int *)0, p);
	p->p_flag &= ~P_SLOCK;
	xp->psx_flag |= XWRIT;
	xp->psx_flag &= ~XLOAD;
	xunlock(&vm_text_list_lock);
}

/*
 * Decrement loaded reference count of text object.  If not sticky and
 * count of zero, attach to LRU cache, at the head if traced or the
 * inode has a hard link count of zero, otherwise at the tail.
 */
void
vm_xfree()
{
	struct user u;
	register vm_text_t xp;
	register struct vnode *vp;
	struct vattr *vattr;

	if ((xp = u->u_procp->p_textp) == NULL) {
		return;
	}
	xstats->psxs_free++;

	xlock(&vm_text_list_lock);
	vp = xp->psx_vptr;
	if (xp->psx_count-- == 0 && (VOP_GETATTR(vp, vattr, u->u_cred, u->u_procp) != 0 || (vattr->va_mode & VSVTX) == 0)) {
		if ((xp->psx_flag & XTRC) || vattr->va_nlink == 0) {
			xp->psx_flag &= ~XLOCK;
			vm_xuntext(xp);
			TAILQ_REMOVE(&vm_text_list, xp, psx_list);
		} else {
			if (xp->psx_flag & XWRIT) {
				xstats->psxs_free_cacheswap++;
				xp->psx_flag |= XUNUSED;
			}
			xstats->psxs_free_cache++;
			xp->psx_ccount--;
			TAILQ_REMOVE(&vm_text_list, xp, psx_list);
		}
	} else {
		xp->psx_ccount--;
		xstats->psxs_free_inuse++;
	}
	xunlock(&vm_text_list_lock);
	u->u_procp->p_textp = NULL;
}

/*
 * Assure core for text segment.  If there isn't enough room to get process
 * in core, swap self out.  x_ccount must be 0.  Text must be locked to keep
 * someone else from freeing it in the meantime.   Don't change the locking,
 * it's correct.
 */
void
vm_xexpand(p, xp)
	struct proc *p;
	vm_text_t xp;
{
	xlock(&vm_text_list_lock);
	if ((xp->psx_caddr = rmalloc(coremap, xp->psx_size)) != NULL) {
		if ((xp->psx_flag & XLOAD) == 0) {
			swap(xp->psx_daddr, xp->psx_caddr, xp->psx_size, xp->psx_vptr, B_READ);
		}
		xunlock(&vm_text_list_lock);
		xp->psx_ccount++;
		return;
	}
	swapout_seg(p, X_FREECORE, X_OLDSIZE, X_OLDSIZE);
	xunlock(&vm_text_list_lock);
	p->p_flag |= SSWAP;
	swtch();
	/* NOTREACHED */
}

/*
 * Decrement the in-core usage count of a shared text segment.
 * When it drops to zero, free the core space.  Write the swap
 * copy of the text if as yet unwritten.
 */
void
vm_xccdec(xp)
	register vm_text_t xp;
{
	if (!xp->psx_ccount) {
		return;
	}
	xlock(&vm_text_list_lock);
	if (--xp->psx_ccount == 0) {
		if (xp->psx_flag & XWRIT) {
			swap(xp->psx_daddr, xp->psx_caddr, xp->psx_size, &swapdev_vp, B_WRITE);
			xp->psx_flag &= ~XWRIT;
		}
		rmfree(coremap, xp->psx_size, xp->psx_caddr);
		xp->psx_caddr = NULL;
	}
	xunlock(&vm_text_list_lock);
}

/*
 * Remove text image from the text table.
 * the use count must be zero.
 */
void
vm_xuntext(xp)
	register vm_text_t xp;
{
	register struct vnode *vp;

	xlock(&vm_text_list_lock);
	if (xp->psx_count == 0) {
		vp = xp->psx_vptr;
		xp->psx_vptr = NULL;
		vm_vsxfree(xp, (long)xp->psx_size);
		//rmfree(swapmap, ctod(xp->psx_size), xp->psx_daddr);
		if (xp->psx_caddr) {
			rmfree(coremap, xp->psx_size, xp->psx_caddr);
		}
		vp->v_flag &= ~VTEXT;
		vp->v_text = NULL;
		vrele(vp);
	}
	xunlock(&vm_text_list_lock);
}

/*
 * Free up "size" core; if swap copy of text has not yet been written,
 * do so.
 */
void
vm_xuncore(size)
	register size_t size;
{
	register vm_text_t xp;

	TAILQ_FOREACH(xp, &vm_text_list, psx_list) {
    	if(!xp->psx_vptr) {
    		continue;
    	}
		xlock(&vm_text_list_lock);
		if (!xp->psx_ccount && xp->psx_caddr) {
			if (xp->psx_flag & XWRIT) {
				swap(xp->psx_daddr, xp->psx_caddr, xp->psx_size, xp->psx_vptr, B_WRITE);
				xp->psx_flag &= ~XWRIT;
			}
			rmfree(coremap, xp->psx_size, xp->psx_caddr);
			xp->psx_caddr = NULL;
			if (xp->psx_size >= size) {
				xunlock(&vm_text_list_lock);
				return;
			}
		}
		xunlock(&vm_text_list_lock);
	}
}

int
vm_xpurge()
{
	register vm_text_t xp;
	int found = 0;

	xstats->psxs_purge++;
	TAILQ_FOREACH(xp, &vm_text_list, psx_list) {
		if (xp->psx_vptr && (xp->psx_flag & (XLOCK|XCACHED)) == XCACHED) {
			vm_xuntext(xp);
			if (xp->psx_vptr == NULL) {
				found++;
			}
		}
	}
	return (found);
}

void
vm_xrele(vp)
	struct vnode *vp;
{
	if (vp->v_flag & VTEXT) {
		vm_xuntext(vp->v_text);
	}
}
