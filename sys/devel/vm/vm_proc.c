/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/buf.h>

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_segment.h>
#include <devel/vm/include/vm_text.h>

/* ARGSUSED */
int
sbrk()
{
	register struct sbrk_args {
		syscallarg(int)	type;
		syscallarg(segsz_t)	size;
		syscallarg(caddr_t)	addr;
		syscallarg(int)	sep;
		syscallarg(int)	flags;
		syscallarg(int) incr;
	} *uap = (struct sbrk_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	register segsz_t n, d;

	p = u.u_procp;

	n = btoc(uap->size);
	if (!SCARG(uap, sep)) {
		SCARG(uap, sep) = PSEG_NOSEP;
	} else {
		n -= ctos(p->p_tsize) * stoc(1);
	}
	if (n < 0) {
		n = 0;
	}
	p->p_tsize;
	if(vm_estabur(p, n, p->p_ssize, p->p_tsize, SCARG(uap, sep), SEG_RO)) {
		return (0);
	}
	vm_segment_expand(p, p->p_psegp, n, S_DATA);
	/* set d to (new - old) */
	d = n - p->p_dsize;
	if (d > 0) {
		clear(p->p_daddr + p->p_dsize, d);	/* fix this: clear as no kern or vm references */
	}
	p->p_dsize = n;
	/* Not yet implemented */
	return (EOPNOTSUPP);	/* return (0) */
}

int
sstk()
{
	register struct sstk_args {
		syscallarg(int) incr;
	} *uap = (struct sstk_args *) u.u_ap;

	return (EOPNOTSUPP);
}

/*
 * Change the size of the data+stack regions of the process.
 * If the size is shrinking, it's easy -- just release the extra core.
 * If it's growing, and there is core, just allocate it and copy the
 * image, taking care to reset registers to account for the fact that
 * the system's stack has moved.  If there is no core, arrange for the
 * process to be swapped out after adjusting the size requirement -- when
 * it comes in, enough core will be allocated.  After the expansion, the
 * caller will take care of copying the user's stack towards or away from
 * the data area.  The data and stack segments are separated from each
 * other.  The second argument to expand specifies which to change.  The
 * stack segment will not have to be copied again after expansion.
 */
void
vm_segment_expand(p, pseg, newsize, type)
	struct proc 	*p;
	vm_psegment_t 	pseg;
	vm_size_t 	 	newsize;
	int 			type;
{
	register vm_size_t i, n;
	caddr_t a1, a2;

	if (type == PSEG_DATA) {
		n = pseg->ps_data.psx_dsize;
		pseg->ps_data.psx_dsize = newsize;
		p->p_dsize = pseg->ps_data.psx_dsize;
		a1 = pseg->ps_data.psx_daddr;
		vm_psegment_expand(pseg, newsize, a1, PSEG_DATA);
		if (n >= newsize) {
			n -= newsize;
			vm_psegment_extent_free(pseg, n + newsize, a1, PSEG_DATA, 0);
			rmfree(coremap, n, a1 + newsize);
			return;
		}
	} else {
		n = pseg->ps_stack.psx_ssize;
		pseg->ps_stack.psx_ssize = newsize;
		p->p_ssize = pseg->ps_stack.psx_ssize;
		a1 = pseg->ps_stack.psx_saddr;
		vm_psegment_expand(pseg, newsize, a1, PSEG_STACK);
		if (n >= newsize) {
			n -= newsize;
			pseg->ps_stack.psx_saddr += n;
			p->p_saddr = pseg->ps_stack.psx_saddr;
			vm_psegment_extent_free(pseg, n, a1, PSEG_STACK, 0);
			rmfree(coremap, n, a1);
			return;
		}
	}
	if (type == PSEG_STACK) {
		a1 = pseg->ps_stack.psx_saddr;
		i = newsize - n;
		a2 = a1 + i;
		/*
		 * i is the amount of growth.  Copy i clicks
		 * at a time, from the top; do the remainder
		 * (n % i) separately.
		 */
		while (n >= i) {
			n -= i;
			bcopy(a1 + n, a2 + n, i);
		}
		bcopy(a1, a2, n);
	}
	a2 = rmalloc(coremap, newsize);
	if (a2 == NULL) {
		if (type == PSEG_DATA) {
			vm_xswapout(p, X_FREECORE, n, X_OLDSIZE);
		} else {
			vm_xswapout(p, X_FREECORE, X_OLDSIZE, n);
		}
	}
	if (type == PSEG_STACK) {
		pseg->ps_stack.psx_saddr = a2;
		p->p_saddr = pseg->ps_stack.psx_saddr;
		/*
		 * Make the copy put the stack at the top of the new area.
		 */
		a2 += newsize - n;
	} else {
		pseg->ps_data.psx_daddr = a2;
		p->p_daddr = pseg->ps_data.psx_daddr;
	}
	bcopy(a1, a2, n);
	rmfree(coremap, n, a1);
}

/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	1.3 (2.11BSD) 2000/2/20
 */
/*
 * Set up software prototype segmentation registers to implement the 3
 * pseudo text, data, stack segment sizes passed as arguments.  The
 * argument sep specifies if the text and data+stack segments are to be
 * separated.  The last argument determines whether the text segment is
 * read-write or read-only.
 */
int
vm_estabur(p, dsize, ssize, tsize, sep, flags)
	struct proc *p;
	segsz_t	 dsize, ssize, tsize;
	int 	 sep, flags;
{
	vm_psegment_t	pseg;

	pseg = p->p_psegp;
	if(pseg) {
		if (estabur(pseg->ps_data, pseg->ps_stack, pseg->ps_text, dsize, ssize, tsize, sep, flags)) {
			return (0);
		}
	}
	return (1);
}

int
estabur(data, stack, text, dsize, ssize, tsize, sep, flags)
	vm_data_t 		data;
	vm_stack_t 		stack;
	vm_text_t 		text;
	segsz_t			dsize, ssize, tsize;
	int 			sep, flags;
{
	if(data == NULL || stack == NULL || text == NULL) {
		return (1);
	}
	switch(sep) {
	case PSEG_NOSEP:
		if (flags == SEG_RO) {
			TEXT_SEGMENT(text, tsize, text->psx_taddr, SEG_RO);
		} else {
			TEXT_SEGMENT(text, tsize, text->psx_taddr, SEG_RW);
		}
		DATA_SEGMENT(data, dsize, data->psx_daddr, SEG_RW);
		STACK_SEGMENT(stack, ssize, stack->psx_saddr, SEG_RW);
		return (0);

	case PSEG_SEP:
		if (flags == SEG_RO) {
			TEXT_SEGMENT(text, tsize, text->psx_taddr, SEG_RO);
		} else {
			TEXT_SEGMENT(text, tsize, text->psx_taddr, SEG_RW);
		}
		DATA_SEGMENT(data, dsize, data->psx_daddr, SEG_RW);
		STACK_SEGMENT(stack, ssize, stack->psx_saddr, SEG_RW);
		return (0);
	}
	return (1);
}

/*
 * Load the user hardware segmentation registers from the software
 * prototype.  The software registers must have been setup prior by
 * estabur.
 */
void
sureg()
{
	int taddr, daddr, saddr;

	taddr = u.u_procp->p_daddr;
	daddr = u.u_procp->p_daddr;
	saddr = u.u_procp->p_saddr;
}

/*
 * Routine to change overlays.  Only hardware registers are changed; must be
 * called from sureg since the software prototypes will be out of date.
 */
void
choverlay(flags)
	int flags;
{

}

/*
 * swap I/O
 */
void
swap(blkno, coreaddr, count, vp, rdflg)
	swblk_t blkno;
	caddr_t coreaddr;
	struct vnode *vp;
	int count, rdflg;
{
	register struct buf *bp;
	register int tcount;

	if (rdflg) {
		cnt.v_pswpin += count;
		cnt.v_pgin++;
	} else {
		cnt.v_pswpout += count;
		cnt.v_pgout++;
	}

	while (count) {
		bp->b_flags = B_BUSY | B_PHYS | B_INVAL | rdflg;
		tcount = count;
		if (tcount >= 01700) {				/* prevent byte-count wrap */
			tcount = 01700;
		}
		bp->b_blkno = blkno;
		if (bp->b_vp) {
			brelvp(bp);
		}
		VHOLD(vp);
		bp->b_vp = vp;
		bp->b_dev = swapdev; 				/* TODO: add support for finding swapdrum */
		bp->b_bcount = ctob(tcount);
		bp->b_un.b_addr = (caddr_t)(coreaddr << 6);
		bp->b_xmem = (coreaddr >> 10) & 077;
		VOP_STRATEGY(bp);
		while ((bp->b_flags & B_DONE) == 0) {
			sleep((caddr_t)bp, PSWP);
		}
		if ((bp->b_flags & B_ERROR) || bp->b_resid) {
			panic("hard err: swap");
		}
		count -= tcount;
		coreaddr += tcount;
		blkno += ctod(tcount);
	}
	brelse(bp);
}

/*
 * rout is the name of the routine where we ran out of swap space.
 */
void
swkill(p, rout)
	register struct proc *p;
	char *rout;
{
	tprintf(u.u_ttyp, "sorry, pid %d killed in %s: no swap space\n", p->p_pid, rout);
}
