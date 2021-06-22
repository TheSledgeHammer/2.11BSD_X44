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
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_text.h>

struct txtlist      vm_text_list;
simple_lock_data_t	vm_text_list_lock;
struct vm_xstats 	*xstats;			/* cache statistics */

/*
 * initialize text table (from vm_segment.c)
 */
void
vm_text_init(pseg, size, flags)
	vm_psegment_t 	*pseg;
    u_long			size;
    int 			flags;
{
    register vm_text_t xp;
    int ntexts;

    simple_lock_init(&vm_text_list_lock, "vm_text_list_lock");

    TAILQ_INIT(&vm_text_list);

    vm_psegment_extent_suballoc(pseg, size, 0, PSEG_TEXT, flags);
}

/*
 * Attach to a shared text segment.  If there is no shared text, just
 * return.  If there is, hook up to it.  If it is not available from
 * core or swap, it has to be read in from the inode (ip); the written
 * bit is set to force it to be written out as appropriate.  If it is
 * not available from core, a swap has to be done to get it back.
 */
void
vm_xalloc(vp, vap)
	register struct vnode *vp;
{
	register vm_text_t xp;

	xp->x_count = 1;
	xp->x_ccount = 0;
	xp->x_vptr = vp;
	vp->v_flag |= VTEXT;
	vp->v_text = xp;

	xp->x_flag |= XWRIT;
	xp->x_flag &= ~XLOAD;
}

/*
 * Decrement loaded reference count of text object.  If not sticky and
 * count of zero, attach to LRU cache, at the head if traced or the
 * inode has a hard link count of zero, otherwise at the tail.
 */
void
vm_xfree()
{
	register vm_text_t xp;
	register struct vnode *vp;
	struct vattr *vattr;

	if ((xp = u->u_procp->p_textp) == NULL) {
		return;
	}

	xstats->xs_free++;

	xlock(&vm_text_list_lock);
	if (xp->x_count-- == 0) {
		if ((xp->x_flag & XTRC) || vattr->va_nlink == 0) {
			xp->x_flag &= ~XLOCK;
			vm_xuntext(xp);
			TAILQ_REMOVE(&vm_text_list, xp, x_list);
		} else {
			if (xp->x_flag & XWRIT) {
				xstats->xs_free_cacheswap++;
				xp->x_flag |= XUNUSED;
			}
			xstats->xs_free_cache++;
		}
	} else {
		xp->x_ccount--;
		xstats->xs_free_inuse++;
	}
	xunlock(&vm_text_list_lock);
	//u->u_procp->p_textp = NULL;
}

/*
 * Assure core for text segment.  If there isn't enough room to get process
 * in core, swap self out.  x_ccount must be 0.  Text must be locked to keep
 * someone else from freeing it in the meantime.   Don't change the locking,
 * it's correct.
 */
void
vm_xexpand(xp)
	register vm_text_t xp;
{
	xlock(&vm_text_list_lock);
	if ((xp->x_caddr = rmalloc(coremap, xp->x_size)) != NULL) {
		if ((xp->x_flag & XLOAD) == 0) {
			//swap();
		}
		xunlock(&vm_text_list_lock);
		xp->x_ccount++;
		return;
	}
	//swapout(u->u_procp, X_FREECORE, X_OLDSIZE, X_OLDSIZE);
	xunlock(&vm_text_list_lock);
	//u->u_procp->p_flag |= SSWAP;
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
	if (!xp->x_ccount) {
		return;
	}
	xlock(&vm_text_list_lock);
	if (--xp->x_ccount == 0) {
		if (xp->x_flag & XWRIT) {
			//swap(xp->x_daddr, xp->x_caddr, xp->x_size, B_WRITE);
			xp->x_flag &= ~XWRIT;
		}
		rmfree(coremap, xp->x_size, xp->x_caddr);
		xp->x_caddr = NULL;
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
	if (xp->x_count == 0) {
		vp = xp->x_vptr;
		xp->x_vptr = NULL;
		rmfree(swapmap, ctod(xp->x_size), xp->x_daddr);
		if (xp->x_caddr) {
			rmfree(coremap, xp->x_size, xp->x_caddr);
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

    TAILQ_FOREACH(xp, &vm_text_list, x_list) {
    	if(!xp->x_vptr) {
    		continue;
    	}
		xlock(&vm_text_list_lock);
		if (!xp->x_ccount && xp->x_caddr) {
			if (xp->x_flag & XWRIT) {
			//	swap(xp->x_daddr, xp->x_caddr, xp->x_size, B_WRITE);
				xp->x_flag &= ~XWRIT;
			}
			rmfree(coremap, xp->x_size, xp->x_caddr);
			xp->x_caddr = NULL;
			if (xp->x_size >= size) {
				xunlock(&vm_text_list_lock);
				return;
			}
		}
		xunlock(&vm_text_list_lock);
	}
}
