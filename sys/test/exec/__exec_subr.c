
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/mman.h>
#include <sys/resourcevar.h>
#include <vm/include/vm.h>
/*
 4.4BSD: getxfile (exec file) vm functions used:
vm_allocate();
vmspace_free();
vmspace_alloc();
vm_map_remove();
vm_map_protect();
vm_mmap(); = uvm_map

TODO: Before can be put into kernel
- Needs to appropriately allocate and free from vmspace.
- vmcmd_map_vmspace
- vmcmd_unmap_vmspace
- map_zero
- readvn (netbsd)
- exec_read
*/

void
new_vmcmd(evsp, elp, addr, size, prot, maxprot, flags, handle, offset)
	struct exec_vmcmd_set *evsp;
	struct exec_linker *elp;
	vm_offset_t *addr;
	vm_size_t size;
	vm_prot_t prot, maxprot;
	int flags;
	caddr_t handle;
	unsigned long offset;
{
	struct exec_vmcmd *vcp;
	if (evsp->evs_used >= evsp->evs_cnt) {
		vmcmdset_extend(evsp);
	}
	vcp = evsp->evs_cmds[evsp->evs_used++];
	vcp ->ev_addr = addr;
	vcp->ev_size = size;
	vcp->ev_prot = prot;
	vcp->ev_maxprot = maxprot;
	vcp->ev_flags = flags;
	if((vcp->ev_handle = handle) != NULL)
		vrele(vcp->ev_handle);
	vcp->ev_offset = offset;

	elp->el_vmcmds = vcp;
}

void
vmcmd_extend(evsp)
	struct exec_vmcmd_set *evsp;
{
	struct exec_vmcmd *nvcp;
	u_int ocnt;

	if (evsp->evs_used < evsp->evs_cnt)
		panic("vmcmdset_extend: not necessary");

	ocnt = evsp->evs_cnt;
	if((ocnt = evsp->evs_cnt) != 0) {
		evsp->evs_cnt += ocnt;
	nvcp = malloc(sizeof(struct exec_vmcmd), M_EXEC, M_WAITOK);
	if (ocnt) {
		memcpy(nvcp, evsp->evs_cmds, (ocnt * sizeof(struct exec_vmcmd)));
		free(evsp->evs_cmds, M_EXEC);
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
		if(vcp->ev_handle != NULL)
			vrele(vcp->ev_handle);
	}
	evsp->evs_used = evsp->evs_cnt = 0;
	free(evsp->evs_cmds, M_EXEC);
}

/* Push exec_mmap into vmspace processing VM information and image params */
int
vmcmd_map_vmspace(elp, entry)
	struct exec_linker *elp;
	u_long entry;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	struct exec_vmcmd *vcp = elp->el_ex_map;
	int error;
	extern struct sysentvec sysvec;

	/* copy in arguments and/or environment from old process */
	error = exec_extract_strings(elp);
	if(error) {
		return (error);
	}

	/* Destroy old process VM and create a new one (with a new stack) */
	exec_new_vmspace(elp);

	error = vm_mmap(&vmspace->vm_map, vcp->ev_addr, vcp->ev_prot, vcp->ev_maxprot, vcp->ev_flags,
			vcp->ev_handle, vcp->ev_offset);
	if (error) {
		return (error);
	}

	error = vm_allocate(&vmspace->vm_map, vcp->ev_addr, vcp->ev_size, FALSE);
	if (error) {
		return (error);
	}

	/* Fill in process VM information */
	vmspace->vm_tsize = btoc(elp->el_tsize);
	vmspace->vm_dsize = btoc(elp->el_dsize);
	vmspace->vm_taddr = (caddr_t) elp->el_taddr;
	vmspace->vm_daddr = (caddr_t) elp->el_daddr;
	vmspace->vm_ssize = btoc(elp->el_ssize);
	vmspace->vm_minsaddr = (caddr_t)elp->el_minsaddr;
	vmspace->vm_maxsaddr = (caddr_t)elp->el_maxsaddr;

	/* Fill in image_params */
	elp->el_interpreted = 0;
	elp->el_entry = entry;
	elp->el_proc->p_sysent = &sysvec;
	return 0;
}

/*
 * Destroy old address space, and allocate a new stack
 *	The new stack is only SGROWSIZ large because it is grown
 *	automatically in trap.c.
 */
int
exec_new_vmspace(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	caddr_t	stack_addr = (caddr_t) (USRSTACK - SGROWSIZ);
	int error;

	elp->el_vmspace_destroyed = 1;
	vm_deallocate(&vmspace->vm_map, 0, USRSTACK);

	/* Allocate a new stack */
	error = vm_allocate(&vmspace->vm_map, (vm_offset_t *)&stack_addr, SGROWSIZ, FALSE);
	if (error) {
		return(error);
	}

	return(0);
}
