/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_swap.c	1.3 (2.11BSD GTE) 3/10/93
 *	@(#)vm_swp.c	2.3 (2.11BSD) 11/30/94
 *	@(#)vm_text.c	1.2 (2.11BSD GTE) 11/26/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/extent.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/tprintf.h>
#include <sys/vnode.h>
#include <sys/user.h>

#include <vm/include/vm.h>
#include <vm/include/vm_text.h>

#include <machine/setjmp.h>

struct txtlist      vm_text_list;
struct vm_xstats 	*xstats;			/* cache statistics */
int					xcache;				/* number of "sticky" texts retained */

void vm_swkill(struct proc *, char *);
void vm_xswap(memaddr_t, memaddr_t, int, struct vnode *, int);

/* lock a virtual text segment */
void
vm_xlock(xp)
	vm_text_t  xp;
{
	simple_lock(&xp->psx_lock);
	while (xp->psx_flag & XLOCK) {
		xp->psx_flag |= XWANT;
		tsleep((caddr_t)&xp, PSWP+1, "xlock", 0);
	}
	xp->psx_flag |= XLOCK;
}

/* unlock a virtual text segment */
void
vm_xunlock(xp)
	vm_text_t 	xp;
{
	if ((xp)->psx_flag & XWANT) {
		wakeup((caddr_t)&xp);
	}
	xp->psx_flag &= ~(XLOCK|XWANT);
	simple_unlock(&xp->psx_lock);
}

/*
 * Wait for xp to be unlocked if it is currently locked.
 */
void
vm_xwait(xp)
	vm_text_t 	xp;
{
	vm_xlock(xp);
	vm_xunlock(xp);
}

/*
 * initialize text table
 */
void
vm_text_init(xp)
	vm_text_t 		xp;
{
    simple_lock_init(&xp->psx_lock, "vm_text_lock");
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
	u_long ts;

	p = vp->v_proc;
	while ((xp == vp->v_text) != NULL) {
		if (xp->psx_flag & XLOCK) {
			vm_xwait(xp);
			continue;
		}
		vm_xlock(xp);
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
		vm_xunlock(xp);
		return;
	}
	xp = TAILQ_FIRST(&vm_text_list);
	if (xp == NULL) {
		tablefull("vm_text");
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
	ts = btoc(tsize);
	xp->psx_size = clrnd(ts);
	/* 2.11BSD overlays were here */
	if((xp->psx_daddr = (caddr_t)rmalloc(swapmap, ctod(xp->psx_size))) == NULL) {
		/* flush text cache and try again */
		vm_swkill(p, "xalloc: no swap space");
		return;
	}
	xp->psx_count = 1;
	xp->psx_ccount = 0;
	xp->psx_vptr = vp;
	vp->v_flag |= VTEXT;
	vp->v_text = xp;
	VREF(vp);
	p->p_textp = xp;
	vm_xexpand(p, xp);
	(void)vm_estabur(p, ts, 0, 0, 0, SEG_RW);
	(void)vn_rdwr(UIO_READ, vp, (caddr_t)ctob(tptov(p, 0)), tsize & ~1, toff, UIO_USERSPACE, (IO_UNIT|IO_NODELOCKED), u.u_ucred, (int *)0, p);
	p->p_flag &= ~P_SLOCK;
	xp->psx_flag |= XWRIT;
	xp->psx_flag &= ~XLOAD;
	vm_xunlock(xp);
}

/*
 * Decrement loaded reference count of text object.  If not sticky and
 * count of zero, attach to LRU cache, at the head if traced or the
 * inode has a hard link count of zero, otherwise at the tail.
 */
void
vm_xfree(void)
{
	register vm_text_t xp;
	register struct vnode *vp;
	struct vattr vattr;

	if ((xp = u.u_procp->p_textp) == NULL) {
		return;
	}
	xstats->psxs_free++;

	vm_xlock(xp);
	vp = xp->psx_vptr;
	if (xp->psx_count-- == 0 && (VOP_GETATTR(vp, &vattr, u.u_ucred, u.u_procp) != 0 || (vattr.va_mode & VSVTX) == 0)) {
		if ((xp->psx_flag & XTRC) || vattr.va_nlink == 0) {
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
	vm_xunlock(xp);
	u.u_procp->p_textp = NULL;
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
	vm_xlock(xp);
	if ((xp->psx_caddr = (caddr_t)rmalloc(coremap, xp->psx_size)) != NULL) {
		if ((xp->psx_flag & XLOAD) == 0) {
			vm_xswap((memaddr_t)xp->psx_daddr, (memaddr_t)xp->psx_caddr, xp->psx_size, xp->psx_vptr, B_READ);
		}
		vm_xunlock(xp);
		xp->psx_ccount++;
		return;
	}
	if (setjmp(&u.u_ssave)) {
		vm_sureg();
		return;
	}
	xswapout(p, X_FREECORE, X_OLDSIZE, X_OLDSIZE);
	vm_xunlock(xp);
	p->p_flag |= P_SSWAP;
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
	vm_xlock(xp);
	if (--xp->psx_ccount == 0) {
		if (xp->psx_flag & XWRIT) {
			vm_xswap((memaddr_t)xp->psx_daddr, (memaddr_t)xp->psx_caddr, xp->psx_size, swapdev_vp, B_WRITE);
			xp->psx_flag &= ~XWRIT;
		}
		rmfree(coremap, xp->psx_size, (memaddr_t)xp->psx_caddr);
		xp->psx_caddr = NULL;
	}
	vm_xunlock(xp);
}

/*
 * Free the swap image of all unused saved-text text segments which are from
 * device dev (used by umount system call).  If dev is NODEV, do all devices
 * (used when rebooting or malloc of swapmap failed).
 */
void
vm_xumount(dev)
	dev_t dev;
{
	register vm_text_t xp;
	TAILQ_FOREACH(xp, &vm_text_list, psx_list) {
		if (xp->psx_vptr != NULL || dev == NODEV) {
			vm_xuntext(xp);
		}
	}
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

	vm_xlock(xp);
	if (xp->psx_count == 0) {
		vp = xp->psx_vptr;
		xp->psx_vptr = NULL;
		rmfree(swapmap, ctod(xp->psx_size), (memaddr_t)xp->psx_daddr);
		if (xp->psx_caddr) {
			rmfree(coremap, xp->psx_size, (memaddr_t)xp->psx_caddr);
		}
		vp->v_flag &= ~VTEXT;
		vp->v_text = NULL;
		vrele(vp);
	}
	vm_xunlock(xp);
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
    	if (!xp->psx_vptr) {
    		continue;
    	}
    	vm_xlock(xp);
		if (!xp->psx_ccount && xp->psx_caddr) {
			if (xp->psx_flag & XWRIT) {
				vm_xswap((memaddr_t)xp->psx_daddr, (memaddr_t)xp->psx_caddr, xp->psx_size, xp->psx_vptr, B_WRITE);
				xp->psx_flag &= ~XWRIT;
			}
			rmfree(coremap, xp->psx_size, (memaddr_t)xp->psx_caddr);
			xp->psx_caddr = NULL;
			if (xp->psx_size >= size) {
				vm_xunlock(xp);
				return;
			}
		}
		vm_xunlock(xp);
	}
}

int
vm_xpurge(void)
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

/*
 * swap I/O
 */
void
vm_xswap(blkno, coreaddr, count, vp, rdflg)
	memaddr_t blkno, coreaddr;
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
vm_swkill(p, rout)
	register struct proc *p;
	char *rout;
{
	tprintf(p->p_session, "sorry, pid %d killed in %s: no swap space\n", p->p_pid, rout);
}

/*
 * Swap a process in.
 * Allocate data and possible text separately.  It would be better
 * to do largest first.  Text, data, stack and u. are allocated in
 * that order, as that is likely to be in order of size.
 */
void
vm_xswapin(p, addr)
	struct proc *p;
	vm_offset_t addr;
{
	register struct vm_text *xp;
	register memaddr_t x;
	memaddr_t a[3];

	x = NULL;

	/* Malloc the text segment first, as it tends to be largest. */
	xp = p->p_textp;
	if (xp) {
		vm_xlock(xp);
		if (!xp->psx_caddr && !xp->psx_ccount) {
			x = rmalloc(coremap, xp->psx_size);
			if (!x) {
				vm_xunlock(xp);
				return;
			}
		}
	}
	if (rmalloc3(coremap, p->p_dsize, p->p_ssize, USIZE, a) == NULL) {
		if (x) {
			rmfree(coremap, xp->psx_size, x);
		}
		if (xp) {
			vm_xunlock(xp);
		}
		return;
	}
	if (xp) {
		if (x) {
			xp->psx_caddr = (caddr_t)x;
			if ((xp->psx_flag & XLOAD) == 0) {
				vm_xswap((memaddr_t)xp->psx_daddr, x, xp->psx_size, p->p_textvp, B_READ);
			}
		}
		xp->psx_ccount++;
		vm_xunlock(xp);
	}
	if (p->p_dsize) {
		vm_xswap((memaddr_t)p->p_daddr, a[0], p->p_dsize, p->p_textvp, B_READ);
		rmfree(swapmap, ctod(p->p_dsize), (memaddr_t)p->p_daddr);
	}
	if (p->p_ssize) {
		vm_xswap((memaddr_t)p->p_saddr, a[1], p->p_ssize, p->p_textvp, B_READ);
		rmfree(swapmap, ctod(p->p_ssize), (memaddr_t)p->p_saddr);
	}
	vm_xswap(addr, a[2], USIZE, p->p_textvp, B_READ);
	rmfree(swapmap, ctod(USIZE), addr);
	p->p_daddr = (caddr_t)a[0];
	p->p_saddr = (caddr_t)a[1];
	addr = a[2];
}

/*
 * Swap out process p.
 * odata and ostack are the old data size and the stack size
 * of the process, and are supplied during core expansion swaps.
 * The freecore flag causes its core to be freed -- it may be
 * off when called to create an image for a child process
 * in newproc.
 *
 * panic: out of swap space
 */
void
vm_xswapout(p, addr, size, freecore, odata, ostack)
	struct proc *p;
	vm_offset_t addr;
	vm_size_t size;
	int freecore;
	register u_int odata, ostack;
{
	memaddr_t a[3];

	if (odata == X_OLDSIZE) {
		odata = p->p_dsize;
	}
	if (ostack == X_OLDSIZE) {
		ostack = p->p_ssize;
	}
	if (rmalloc3(swapmap, ctod(p->p_dsize), ctod(p->p_ssize), ctod(USIZE), a) == NULL) {
		panic("out of swap space");
	}
	if (p->p_textp) {
		vm_xccdec(p->p_textp);
	}
	if (odata) {
		vm_xswap(a[0], (memaddr_t)p->p_daddr, odata, p->p_textvp, B_WRITE);
		if (freecore == X_FREECORE) {
			rmfree(coremap, odata, (memaddr_t)p->p_daddr);
		}
	}
	if (ostack) {
		vm_xswap(a[1], (memaddr_t)p->p_saddr, ostack, p->p_textvp, B_WRITE);
		if (freecore == X_FREECORE) {
			rmfree(coremap, ostack, (memaddr_t)p->p_saddr);
		}
	}
	vm_xswap(a[2], addr, USIZE, p->p_textvp, B_WRITE);
	if (freecore == X_FREECORE) {
		rmfree(coremap, USIZE, addr);
	}
	p->p_daddr = (caddr_t)a[0];
	p->p_saddr = (caddr_t)a[1];
	addr = a[2];
}

/*
 * Get text structures.  This is a 2.11BSD extension.  sysctl() is supposed
 * to be extensible...
 */
int
sysctl_text(where, sizep)
	char *where;
	size_t *sizep;
{
	int buflen, error;
	register struct vm_text *xp;
	struct vm_text *xpp;
	char *start = where;
	register int i;

	buflen = *sizep;
	if (where == NULL) {
		vm_xlock(xp);
		for (i = 0, xp = TAILQ_FIRST(&vm_text_list); xp != NULL; xp = TAILQ_NEXT(xp, psx_list)) {
			if (xp->psx_count) {
				i++;
			}
#define	TPTRSZ	sizeof(struct vm_text *)
#define	TEXTSZ	sizeof(struct vm_text)
		/*
		 * overestimate by 3 text structures
		 */
		*sizep = (i + 3) * (TEXTSZ + TPTRSZ);
		}
		vm_xunlock(xp);
		return (0);
	}

	/*
	 * array of extended file structures: first the address then the
	 * file structure.
	 */
	vm_xlock(xp);
	TAILQ_FOREACH(xp, &vm_text_list, psx_list) {
		if (xp->psx_count == 0) {
			continue;
		}
		if (buflen < (TPTRSZ + TEXTSZ)) {
			*sizep = where - start;
			return (ENOMEM);
		}
		xpp = xp;
		if ((error = copyout(&xpp, where, TPTRSZ))
				|| (error = copyout(xp, where + TPTRSZ, TEXTSZ))) {
			vm_xunlock(xp);
			return (error);
		}
		buflen -= (TPTRSZ + TEXTSZ);
		where += (TPTRSZ + TEXTSZ);
	}
	*sizep = where - start;
	vm_xunlock(xp);
	return (0);
}
