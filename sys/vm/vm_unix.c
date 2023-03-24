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
 */

/*
 * Traditional sbrk/grow interface to VM
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>

#include <vm/include/vm.h>
#include <vm/include/vm_text.h>

int estabur(vm_data_t, vm_stack_t, vm_text_t, segsz_t, segsz_t, segsz_t, int, int);
int	ogrow(vm_offset_t);

struct obreak_args {
	char	*nsiz;
};
/* ARGSUSED */
int
obreak(p, uap, retval)
	struct proc *p;
	struct obreak_args *uap;
	int *retval;
{
	register struct vmspace *vm;
	vm_offset_t new, old;
	int rv;
	register int diff;

	vm = p->p_vmspace;
	old = (vm_offset_t)vm->vm_daddr;
	new = round_page(uap->nsiz);
	if ((int)(new - old) > p->p_rlimit[RLIMIT_DATA].rlim_cur)
		return(ENOMEM);
	old = round_page(old + ctob(vm->vm_dsize));
	diff = new - old;
	if (diff > 0) {
		rv = vm_allocate(&vm->vm_map, &old, diff, FALSE);
		if (rv != KERN_SUCCESS) {
			uprintf("sbrk: grow failed, return = %d\n", rv);
			return(ENOMEM);
		}
		vm->vm_dsize += btoc(diff);
	} else if (diff < 0) {
		diff = -diff;
		rv = vm_deallocate(&vm->vm_map, new, diff);
		if (rv != KERN_SUCCESS) {
			uprintf("sbrk: shrink failed, return = %d\n", rv);
			return(ENOMEM);
		}
		vm->vm_dsize -= btoc(diff);
	}
	return(0);
}

/*
 * Enlarge the "stack segment" to include the specified
 * stack pointer for the process.
 */
int
grow(p, sp)
	struct proc *p;
	vm_offset_t sp;
{
	register struct vmspace *vm;
	register int si;

	vm = p->p_vmspace;
	/*
	 * For user defined stacks (from sendsig).
	 */
	if (sp < (vm_offset_t)vm->vm_maxsaddr) {
		return (0);
	}
	/*
	 * For common case of already allocated (from trap).
	 */
	if (sp >= USRSTACK - ctob(vm->vm_ssize)) {
		return (1);
	}
	/*
	 * Really need to check vs limit and increment stack size if ok.
	 */
	si = clrnd(btoc(USRSTACK-sp) - vm->vm_ssize);
	if (vm->vm_ssize + si > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
		return (0);
	}
	vm->vm_ssize += si;
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
	vm_psegment_t	pseg;

	pseg = p->p_vmspace->vm_psegment;
	if (pseg == NULL) {
		return (1);
	}
	if (estabur(pseg->ps_data, pseg->ps_stack, pseg->ps_text, dsize, ssize, tsize, sep, flags)) {
		return (0);
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
	vm_text_t tp;
	int taddr, daddr, saddr;

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
 * called from sureg since the software prototypes will be out of date.
 */
void
choverlay(flags)
	int flags;
{

}

/*
 * grow the stack to include the SP
 * true return if successful.
 */
int
ogrow(sp)
	vm_offset_t sp;
{
	register int si;

	if (sp >= -ctob(u.u_ssize)) {
		return (0);
	}
	si = (-sp) / ctob(1) - u.u_ssize + SINCR;
	/*
	 * Round the increment back to a segment boundary if necessary.
	 */
	if (ctos(si + u.u_ssize) > ctos(((-sp) + ctob(1) - 1) / ctob(1))) {
		si = stoc(ctos(((-sp) + ctob(1) - 1) / ctob(1))) - u.u_ssize;
	}
	if (si <= 0) {
		return (0);
	}
	if (vm_estabur(u.u_procp, u.u_tsize, u.u_dsize, u.u_ssize + si, u.u_sep, SEG_RO)) {
		return (0);
	}

	/*
	 *  expand will put the stack in the right place;
	 *  no copy required here.
	 */
	vm_expand(u.u_procp, u.u_ssize + si, PSEG_STACK);
	u.u_ssize += si;
	bzero(u.u_procp->p_saddr, si);
	return (1);
}
