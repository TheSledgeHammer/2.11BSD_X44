/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_text.c	1.2 (2.11BSD GTE) 11/26/94
 */

#include <sys/extent.h>
#include <sys/malloc.h>
#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_text.h>

struct txtlist      vm_text_list;
simple_lock_data_t	vm_text_list_lock;

/*
 * initialize text table (from vm_segment.c)
 */
void
vm_text_init(vm, segment, start, end, flags)
	struct vmspace 	*vm;
    vm_segment_t 	segment;
    vm_size_t		start, end;
    int 			flags;
{
    register vm_text_t xp;
    register vm_sregion_t *sreg;
    int ntexts;

    simple_lock_init(&vm_text_list_lock, "vm_text_list_lock");

    TAILQ_INIT(&vm_text_list);

    sreg = vm_region_create(vm, segment, "vm_text", start, end, M_COREMAP, EX_WAITOK);

}

/*
 * Attach to a shared text segment.  If there is no shared text, just
 * return.  If there is, hook up to it.  If it is not available from
 * core or swap, it has to be read in from the inode (ip); the written
 * bit is set to force it to be written out as appropriate.  If it is
 * not available from core, a swap has to be done to get it back.
 */
void
vm_xalloc(vp)
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
	u->u_procp->p_flag |= SSWAP;
	swtch();
	/* NOTREACHED */
}
