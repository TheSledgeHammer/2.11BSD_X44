/*	$NetBSD: exec_subr.c,v 1.18.2.2 2000/11/05 22:43:40 tv Exp $	*/

/*
 * Copyright (c) 1993, 1994, 1996 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
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
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/mman.h>
#include <sys/stdbool.h>
#include <sys/resourcevar.h>

#include <vm/include/vm.h>
#include <vm/include/vm_object.h>
#include <vm/include/vm_pager.h>
#include <vm/include/vm_param.h>

#include <vm/include/vm_psegment.h>

/* revised to use vm pseudo segments */

/*
 * Creates new vmspace from running vmcmd
 * Do whatever is necessary to prepare the address space
 * for remapping.  Note that this might replace the current
 * vmspace with another!
 */
int
vmcmd_create_vmspace(elp)
	struct exec_linker *elp;
{
	struct proc *p;
	struct vmspace *vmspace;
	vm_psegment_t 	psegment;
	struct exec_vmcmd *base_vcp;
	int error, i;

	p = elp->el_proc;
	vmspace = p->p_vmspace;
	psegment = &vmspace->vm_psegment;
	base_vcp = NULL;

	/*  Now map address space */
	psegment->ps_tsize = btoc(elp->el_tsize);
	psegment->ps_dsize = btoc(elp->el_dsize);
	psegment->ps_taddr = (caddr_t) elp->el_taddr;
	psegment->ps_daddr = (caddr_t) elp->el_daddr;
	psegment->ps_ssize = btoc(elp->el_ssize);
	psegment->ps_minsaddr = (caddr_t) elp->el_minsaddr;
	psegment->ps_maxsaddr = (caddr_t) elp->el_maxsaddr;

	/* create the new process's VM space by running the vmcmds */
#ifdef DIAGNOSTIC
	if(elp->el_vmcmds.evs_used == 0)
		panic("execve: no vmcmds");
#endif
	for (i = 0; i < elp->el_vmcmds.evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = &elp->el_vmcmds.evs_cmds[i];
		if (vcp->ev_flags & VMCMD_RELATIVE) {
#ifdef DIAGNOSTIC
			if (base_vcp == NULL)
				panic("execve: relative vmcmd with no base");
			if (vcp->ev_flags & VMCMD_BASE)
				panic("execve: illegal base & relative vmcmd");
#endif
			vcp->ev_addr += base_vcp->ev_addr;
		}
		error = (*vcp->ev_proc)(elp->el_proc, vcp);
#ifdef DEBUG
		if (error) {
			if (i > 0) {
				printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i-1,
								       vcp[-1].ev_addr, vcp[-1].ev_size,
								       vcp[-1].ev_offset);
			}
			printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i, vcp->ev_addr, vcp->ev_size, vcp->ev_offset);
		}
#endif
		if (vcp->ev_flags & VMCMD_BASE)
			base_vcp = vcp;
	}

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmd(&elp->el_vmcmds);
	if (error) {
		return (error);
	} else {
		return (0);
	}
}
