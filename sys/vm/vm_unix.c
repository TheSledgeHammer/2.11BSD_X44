/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: vm_unix.c 1.1 89/11/07$
 *
 *	@(#)vm_unix.c	8.2 (Berkeley) 1/9/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_mman.c	1.3 (2.11BSD) 2000/2/20
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

/*
 * Traditional sbrk/grow interface to VM
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/map.h>
#include <sys/sysdecl.h>

#include <vm/include/vm.h>
#include <vm/include/vm_text.h>
#include <vm/include/vm_psegment.h>

#include <machine/setjmp.h>

static int 	estabur(vm_data_t, vm_stack_t, vm_text_t, segsz_t, segsz_t, segsz_t, int, int);

/* ARGSUSED */
int
obreak()
{
	register struct obreak_args {
		syscallarg(char	*)nsiz;
	} *uap = (struct obreak_args *)u.u_ap;
	struct proc *p;
	int *retval;
	register struct vmspace *vm;
	vm_offset_t new, old;
	int rv;
	register int diff;

	p = u.u_procp;
	vm = p->p_vmspace;
	old = (vm_offset_t)vm->vm_daddr;
	new = (vm_offset_t)round_page(SCARG(uap, nsiz));
	if ((int)(new - old) > p->p_rlimit[RLIMIT_DATA].rlim_cur)
		return (ENOMEM);
	old = round_page(old + ctob(vm->vm_dsize));
	diff = new - old;
	if (diff > 0) {
		rv = vm_allocate(&vm->vm_map, &old, diff, FALSE);
		if (rv != KERN_SUCCESS) {
			uprintf("sbrk: grow failed, return = %d\n", rv);
			return (ENOMEM);
		}
		vm->vm_dsize += btoc(diff);
	} else if (diff < 0) {
		diff = -diff;
		rv = vm_deallocate(&vm->vm_map, new, diff);
		if (rv != KERN_SUCCESS) {
			uprintf("sbrk: shrink failed, return = %d\n", rv);
			return (ENOMEM);
		}
		vm->vm_dsize -= btoc(diff);
	}
	return (0);
}

/*
 * set up the process to have no stack segment.  The process is
 * responsible for the management of its own stack, and can thus
 * use the full 64K byte address space.
 */
int
nostk()
{
	void *uap = u.u_ap;
	struct proc *p;
	register struct vmspace *vm;

	p = u.u_procp;
	vm = p->p_vmspace;
	if (vm_estabur(p, 0, u.u_ssize, u.u_tsize, u.u_sep, SEG_RO)) {
		return (1);
	}
	vm_expand(p, 0, PSEG_STACK);
	vm->vm_ssize = 0;
	u.u_ssize = vm->vm_ssize;
	return (0);
}

/*
 * Enlarge the "stack segment" to include the specified
 * stack pointer for the process.
 *
 * grow the stack to include the SP
 * true return if successful.
 */
int
grow(p, sp)
	struct proc *p;
	vm_offset_t sp;
{
	register struct vmspace *vm;
	register int si;
	int usi, vsi;

	vm = p->p_vmspace;

	/*
	 * For user defined stacks (from sendsig).
	 */
	if (sp < (vm_offset_t) vm->vm_maxsaddr || sp >= -ctob(u.u_ssize)) {
		return (0);
	}

	/*
	 * For common case of already allocated (from trap).
	 */
	if (sp >= USRSTACK - ctob(vm->vm_ssize)) {
		return (1);
	}

	usi = (-sp) / ctob(1) - u.u_ssize + SINCR;

	/*
	 * Round the increment back to a segment boundary if necessary.
	 */
	if (ctos(usi + u.u_ssize) > ctos(((-sp) + ctob(1) - 1) / ctob(1))) {
		usi = stoc(ctos(((-sp) + ctob(1) - 1) / ctob(1))) - u.u_ssize;
	}

	/*
	 * Really need to check vs limit and increment stack size if ok.
	 */
	vsi = clrnd(btoc(USRSTACK - sp) - vm->vm_ssize);
	if (usi == vsi) {
		si = usi;
	} else {
		si = vsi;
	}
	if (vm->vm_ssize + si > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
		return (0);
	}
	if (si <= 0) {
		return (0);
	}

	if (vm_estabur(p, u.u_dsize, u.u_ssize + si, u.u_tsize, u.u_sep, SEG_RO)) {
		return (0);
	}

	/*
	 *  expand will put the stack in the right place;
	 *  no copy required here.
	 */
	vm_expand(p, u.u_ssize + si, PSEG_STACK);

	vm->vm_ssize += si;
	u.u_ssize = vm->vm_ssize;
	bzero(p->p_saddr, si);
	return (1);
}

struct ovadvise_args {
	int	anom;
};
/* ARGSUSED */
int
ovadvise(p, uap, retval)
	struct proc *p;
	struct ovadvise_args *uap;
	int *retval;
{
	return (EINVAL);
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
vm_expand(p, newsize, type)
	struct proc 	*p;
	vm_size_t 	 	newsize;
	int 			type;
{
	register vm_pseudo_segment_t pseg;
	register vm_size_t i, n;
	caddr_t a1, a2;

	pseg = &p->p_vmspace->vm_psegment;
	if (pseg == NULL) {
		return;
	}
	if (type == PSEG_DATA) {
		n = pseg->ps_dsize;
		pseg->ps_dsize = newsize;
		p->p_dsize = pseg->ps_dsize;
		a1 = pseg->ps_daddr;
		vm_psegment_expand(pseg, newsize, a1, PSEG_DATA);
		if (n >= newsize) {
			n -= newsize;
			vm_psegment_free(pseg, coremap, n, a1 + newsize, PSEG_DATA);
			return;
		}
	} else {
		n = pseg->ps_ssize;
		pseg->ps_ssize = newsize;
		p->p_ssize = pseg->ps_ssize;
		a1 = pseg->ps_saddr;
		vm_psegment_expand(pseg, newsize, a1, PSEG_STACK);
		if (n >= newsize) {
			n -= newsize;
			pseg->ps_saddr += n;
			p->p_saddr = pseg->ps_saddr;
			vm_psegment_free(pseg, coremap, n, a1, PSEG_STACK);
			/*
			 *  Since the base of stack is different,
			 *  segmentation registers must be repointed.
			 */
			vm_sureg();
			return;
		}
	}
	if (setjmp(&u.u_ssave)) {
		/*
		 * If we had to swap, the stack needs moving up.
		 */
		if (type == PSEG_STACK) {
			a1 = pseg->ps_saddr;
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
		vm_sureg();
		return;
	}
	if (u.u_fpsaved == 0) {
		//savfp(&u.u_fps);
		u.u_fpsaved = 1;
	}
	a2 = (caddr_t)rmalloc(coremap, newsize);
	if (a2 == NULL) {
		if (type == PSEG_DATA) {
			xswapout(p, X_FREECORE, n, X_OLDSIZE);
		} else {
			xswapout(p, X_FREECORE, X_OLDSIZE, n);
		}
		p->p_flag |= P_SSWAP;
		swtch();
		/* NOTREACHED */
	}
	if (type == PSEG_STACK) {
		pseg->ps_saddr = a2;
		p->p_saddr = pseg->ps_saddr;
		/*
		 * Make the copy put the stack at the top of the new area.
		 */
		a2 += newsize - n;
	} else {
		pseg->ps_daddr = a2;
		p->p_daddr = pseg->ps_daddr;
	}
	bcopy(a1, a2, n);
	rmfree(coremap, n, (long)a1);
	vm_sureg();
}

/*
 * Set up software prototype segmentation registers to implement the 3
 * pseudo text, data, stack segment sizes passed as arguments.  The
 * argument sep specifies if the text and data+stack segments are to be
 * separated.  The last argument determines whether the text segment is
 * read-write or read-only.
 */
int
vm_estabur(p, dsize, ssize, tsize, sep, flags)
	struct proc		*p;
	segsz_t	 		dsize, ssize, tsize;
	int 	 		sep, flags;
{
	register struct vmspace *vm;
	vm_pseudo_segment_t	pseg;

	vm = p->p_vmspace;
	pseg = &vm->vm_psegment;
	if (pseg == NULL) {
		return (ENOMEM);
	}
	if (estabur(pseg->ps_data, pseg->ps_stack, pseg->ps_text, dsize, ssize, tsize, sep, flags)) {
		return (0);
	}
	return (ENOMEM);
}

static int
estabur(data, stack, text, dsize, ssize, tsize, sep, flags)
	vm_data_t 		data;
	vm_stack_t 		stack;
	vm_text_t 		text;
	segsz_t			dsize, ssize, tsize;
	int 			sep, flags;
{
	if (data == NULL || stack == NULL || text == NULL) {
		return (ENOMEM);
	}
	switch (sep) {
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
	return (ENOMEM);
}

/*
 * Load the user hardware segmentation registers from the software
 * prototype.  The software registers must have been setup prior by
 * estabur.
 */
void
vm_sureg()
{
	vm_text_t tp;
	caddr_t taddr, daddr, saddr;

	taddr = u.u_procp->p_daddr;
	daddr = u.u_procp->p_daddr;
	saddr = u.u_procp->p_saddr;
	tp = u.u_procp->p_textp;

	if (tp != NULL) {
		taddr = tp->psx_caddr;
	}
}

/*
 * Routine to change overlays.  Only hardware registers are changed; must be
 * called from vm_sureg since the software prototypes will be out of date.
 */
void
choverlay(flags)
	int flags;
{

}
