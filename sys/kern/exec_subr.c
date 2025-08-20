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

void
new_vmcmd(evsp, proc, size, addr, prot, maxprot, flags, vnode, offset)
	struct exec_vmcmd_set *evsp;
	int (*proc)(struct proc *, struct exec_vmcmd *);
	u_long size, addr;
	u_int prot, maxprot;
	int flags;
	struct vnode *vnode;
	u_long offset;
{
	struct exec_vmcmd *vcp;

	if (evsp->evs_used >= evsp->evs_cnt) {
		vmcmd_extend(evsp);
	}
	vcp = &evsp->evs_cmds[evsp->evs_used++];
	vcp->ev_proc = proc;
	vcp->ev_size = size;
	vcp->ev_addr = addr;
	vcp->ev_prot = prot;
	vcp->ev_maxprot = maxprot;
	vcp->ev_flags = flags;
	if ((vcp->ev_vnodep = vnode) != NULL) {
		vref(vnode);
	}
	vcp->ev_offset = offset;
}

void
vmcmd_extend(evsp)
	struct exec_vmcmd_set *evsp;
{
	struct exec_vmcmd *nvcp;
	u_int ocnt;
#ifdef DIAGNOSTIC
	if (evsp->evs_used < evsp->evs_cnt)
		panic("vmcmdset_extend: not necessary");
#endif
	if ((ocnt = evsp->evs_cnt) != 0) {
		evsp->evs_cnt += ocnt;
	} else {
		evsp->evs_cnt = EXEC_DEFAULT_VMCMD_SETSIZE;
	}
	MALLOC(nvcp, struct exec_vmcmd *, (evsp->evs_cnt * sizeof(struct exec_vmcmd)), M_EXEC, M_WAITOK);
	if (ocnt) {
		bcopy(evsp->evs_cmds, nvcp, (ocnt * sizeof(struct exec_vmcmd)));
		FREE(evsp->evs_cmds, M_EXEC);
	}
	evsp->evs_cmds = nvcp;
}

void
kill_vmcmd(evsp)
	struct exec_vmcmd_set *evsp;
{
	struct exec_vmcmd *vcp;
	int i;

	if (evsp->evs_cnt == 0)
		return;

	for (i = 0; i < evsp->evs_used; i++) {
		vcp = &evsp->evs_cmds[i];
		if (vcp->ev_vnodep != NULL) {
			vrele(vcp->ev_vnodep);
		}
	}
	evsp->evs_used = evsp->evs_cnt = 0;
	FREE(evsp->evs_cmds, M_EXEC);
}

int
vmcmd_map_object(p, cmd)
	struct proc *p;
	struct exec_vmcmd *cmd;
{
	struct vmspace *vmspace;
	struct vnode *vp;
	struct pager_struct *vpgr;
	struct vm_object *vobj;
	int error, anywhere;

	vmspace = p->p_vmspace;
	vp = cmd->ev_vnodep;
	anywhere = 0;
	/* vm pager */
	vpgr = vm_pager_allocate(PG_VNODE, (caddr_t)vp, cmd->ev_size,
			VM_PROT_READ | VM_PROT_EXECUTE, cmd->ev_offset);
	if (vpgr == NULL) {
		error = ENOMEM;
		goto bad;
	}
	/* vm object */
	vobj = vm_object_lookup(vpgr);
	if (vobj == NULL) {
		vobj = vm_object_allocate(cmd->ev_size);
		vm_object_enter(vobj, vpgr);
	}
	/* vm map */
	error = vm_map_find(&vmspace->vm_map, vobj, cmd->ev_offset, cmd->ev_addr, cmd->ev_size, anywhere);
	if (error != 0) {
		vm_object_deallocate(vobj);
		goto bad;
	}
	if (vpgr != NULL) {
		vm_object_setpager(vobj, vpgr, 0, TRUE);
	}
	return (error);

bad:
	if (vpgr != NULL) {
		vm_pager_deallocate(vpgr);
	}
	return (error);
}

int
vmcmd_map_pagedvn(p, cmd)
	struct proc *p;
	struct exec_vmcmd *cmd;
{
	struct vmspace *vmspace;
	struct vnode *vp;
	int error;
	vm_prot_t prot, maxprot;

	/*
	 * map the vnode in using vm_mmap.
	 */
	vmspace = p->p_vmspace;
	vp = cmd->ev_vnodep;

	/* checks imported from vm_mmap, needed? */
	if (cmd->ev_size == 0)
		return (0);
	if (cmd->ev_offset & PAGE_MASK)
		return (EINVAL);
	if (cmd->ev_addr & PAGE_MASK)
		return (EINVAL);
	if (cmd->ev_size & PAGE_MASK)
		return (EINVAL);

	prot = cmd->ev_prot;
	maxprot = VM_PROT_ALL;

	/*
	 * check the file system's opinion about mmapping the file
	 */
	error = VOP_MMAP(vp, prot, p->p_ucred, p);
	if (error) {
		return (error);
	}

	if (vp->v_flag == 0) {
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
		VOP_UNLOCK(vp, 0, p);
	}

	vref(vp);

	/*
	 * do the map
	 */
	return (vm_mmap(&vmspace->vm_map, &cmd->ev_addr, cmd->ev_len, prot, maxprot, (MAP_FIXED | MAP_COPY), (caddr_t)vp, cmd->ev_offset));
}

int
vmcmd_map_readvn(p, cmd)
	struct proc *p;
	struct exec_vmcmd *cmd;
{
	struct vmspace *vmspace;
	int error;

	vmspace = p->p_vmspace;

	if (cmd->ev_size == 0) {
		return (0);
	}

	error = vm_allocate(&vmspace->vm_map, &cmd->ev_addr, cmd->ev_size, 0);
	if (error) {
		return (error);
	}
	return (vmcmd_readvn(p, cmd));
}

int
vmcmd_readvn(p, cmd)
	struct proc *p;
	struct exec_vmcmd *cmd;
{
	struct vmspace *vmspace;
	int error;
	vm_prot_t prot, maxprot;

	vmspace = p->p_vmspace;
	error = vn_rdwr(UIO_READ, cmd->ev_vnodep, (caddr_t) cmd->ev_addr,
			cmd->ev_size, cmd->ev_offset, UIO_USERSPACE, IO_UNIT, p->p_ucred,
			NULL, p);
	if (error) {
		return (error);
	}

	prot = cmd->ev_prot;
	maxprot = VM_PROT_ALL;

	if (maxprot != VM_PROT_ALL) {
		error = vm_map_protect(&vmspace->vm_map, trunc_page(cmd->ev_addr),
				round_page(cmd->ev_addr + cmd->ev_size), maxprot, TRUE);
		if (error) {
			return (error);
		}
	}

	if (prot != maxprot) {
		error = vm_map_protect(&vmspace->vm_map, trunc_page(cmd->ev_addr),
				round_page(cmd->ev_addr + cmd->ev_size), prot, FALSE);
		if (error) {
			return (error);
		}
	}
	return (0);
}

/*
 * vmcmd_map_zero():
 *	handle vmcmd which specifies a zero-filled address space region.  The
 *	address range must be first allocated, then protected appropriately.
 */
int
vmcmd_map_zero(p, cmd)
	struct proc *p;
	struct exec_vmcmd *cmd;
{
	struct vmspace *vmspace;
	int error;
	vm_prot_t prot;

	vmspace = p->p_vmspace;
	prot = cmd->ev_prot;
	error = vm_allocate(&vmspace->vm_map, &cmd->ev_addr, cmd->ev_size, 0);
	if (error) {
		return (error);
	}
	return (vm_map_protect(&vmspace->vm_map, trunc_page(cmd->ev_addr), round_page(cmd->ev_addr + cmd->ev_size), prot, FALSE));
}

/*
 * Creates new vmspace from running vmcmd
 * Do whatever is necessary to prepare the address space
 * for remapping.  Note that this might replace the current
 * vmspace with another!
 */
int
vmcmd_create_vmspace(p, elp, base_vcp)
	struct proc *p;
	struct exec_linker *elp;
	struct exec_vmcmd *base_vcp;
{
	struct vmspace *vmspace;
	int error, i;

	vmspace = p->p_vmspace;

	/*  Now map address space */
	vmspace->vm_tsize = btoc(elp->el_tsize);
	vmspace->vm_dsize = btoc(elp->el_dsize);
	vmspace->vm_taddr = (caddr_t) elp->el_taddr;
	vmspace->vm_daddr = (caddr_t) elp->el_daddr;
	vmspace->vm_ssize = btoc(elp->el_ssize);
	vmspace->vm_minsaddr = (caddr_t) elp->el_minsaddr;
	vmspace->vm_maxsaddr = (caddr_t) elp->el_maxsaddr;

	/* create the new process's VM space by running the vmcmds */
#ifdef DIAGNOSTIC
	if (elp->el_vmcmds.evs_used == 0) {
		panic("execve: no vmcmds");
	}
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
		if (vcp->ev_flags & VMCMD_BASE) {
			base_vcp = vcp;
		}
	}

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmd(&elp->el_vmcmds);
	if (error) {
		return (error);
	} else {
		return (0);
	}
}

int
exec_read_from(p, vp, off, bf, size)
	struct proc *p;
	struct vnode *vp;
	u_long off;
	void *bf;
	size_t size;
{
	int error;
	size_t resid;

	if ((error = vn_rdwr(UIO_READ, vp, bf, size, off, UIO_SYSSPACE, 0, p->p_ucred, &resid, NULL)) != 0) {
		return (error);
	}
	/*
	 * See if we got all of it
	 */
	if (resid != 0) {
		return (ENOEXEC);
	}
	return (0);
}

int
exec_setup_stack(elp)
	struct exec_linker *elp;
{
	u_long max_stack_size;
	u_long access_linear_min, access_size;
	u_long noaccess_linear_min, noaccess_size;

	max_stack_size = MAXSSIZ;
	elp->el_minsaddr = (caddr_t)USRSTACK;
	elp->el_maxsaddr = elp->el_minsaddr + max_stack_size;

	elp->el_ssize = u.u_rlimit[RLIMIT_STACK].rlim_cur;

	access_size = elp->el_ssize;
	access_linear_min = (u_long) (elp->el_minsaddr - access_size);
	noaccess_size = max_stack_size - access_size;
	noaccess_linear_min = (u_long) ((elp->el_minsaddr - access_size)
			+ noaccess_size);
	noaccess_size = max_stack_size - access_size;

	if (noaccess_size > 0) {
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, noaccess_size,
				noaccess_linear_min, VM_PROT_NONE, VM_PROT_NONE, NULL, 0);
	}

	NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, access_size, noaccess_linear_min,
			(VM_PROT_READ | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), NULL, 0);

	return (0);
}
