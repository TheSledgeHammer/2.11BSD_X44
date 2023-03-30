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

#include <devel/vm/include/vm.h>
#include <devel/vm/include/vm_text.h>

int estabur(vm_data_t, vm_stack_t, vm_text_t, segsz_t, segsz_t, segsz_t, int, int);
int	ogrow(vm_offset_t);

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
	register vm_psegment_t 	pseg;
	register vm_size_t i, n;
	caddr_t a1, a2;

	pseg = &p->p_vmspace->vm_psegment;
	if (pseg == NULL) {
		return;
	}
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
	a2 = (caddr_t)rmalloc(coremap, newsize);
	if (a2 == NULL) {
		if (type == PSEG_DATA) {
			xswapout(p, X_FREECORE, n, X_OLDSIZE);
		} else {
			xswapout(p, X_FREECORE, X_OLDSIZE, n);
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
	vm_psegment_t	pseg;

	vm = p->p_vmspace;
	pseg = &vm->vm_psegment;
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
