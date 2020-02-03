
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/mman.h>
#include <sys/resourcevar.h>
#include <vm/include/vm.h>
#include <exec/__exec.h>
#include <exec/__exec_linker.h>

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

int
vmcmd_map_pagedvn(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	struct exec_vmcmd *cmd = elp->el_vmcmds->evs_cmds;
	struct vm_object *vobj;
	int retval;

	/*
	 * map the vnode in using vm_mmap.
	 */

	/* checks imported from vm_mmap, needed? */
	if (cmd->ev_size == 0)
		return(0);
	if (cmd->ev_offset & PAGE_MASK)
		return(EINVAL);
	if (cmd->ev_addr & PAGE_MASK)
		return(EINVAL);
	if (cmd->ev_size & PAGE_MASK)
		return(EINVAL);

	vobj = vnode_pager_alloc(cmd->ev_handle, cmd->ev_size, VM_PROT_READ|VM_PROT_EXECUTE, cmd->ev_offset);
	if(vobj == NULL) {
		return (ENOMEM);
	}

	/*
	 * do the map
	 */
	retval = vm_mmap(&vmspace->vm_map, cmd->ev_addr, cmd->ev_size, cmd->ev_prot, cmd->ev_maxprot, cmd->ev_flags, cmd->ev_handle, cmd->ev_offset);

	/*
	 * check for error
	 */

	if (retval == KERN_SUCCESS)
		return(0);

	/*
	 * error: detach from object
	 */
	vobj->pager->pg_ops->pgo_dealloc(vobj);
	return (EINVAL);
}


int
vmcmd_map_readvn(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	struct exec_vmcmd *cmd = elp->el_vmcmds->evs_cmds;

	int error;
	long diff;

	if (cmd->ev_size == 0)
			return(KERN_SUCCESS); /* XXXCDC: should it happen? */
	diff = cmd->ev_addr - trunc_page(cmd->ev_addr);
	cmd->ev_addr -= diff;
	cmd->ev_offset -= diff;
	cmd->ev_size += diff;

	error = vm_mmap(&vmspace->vm_map, cmd->ev_addr, cmd->ev_size, cmd->ev_prot, cmd->ev_maxprot, cmd->ev_flags,
			cmd->ev_handle, cmd->ev_offset);
	if (error)
		return error;

	return vmcmd_readvn(elp, cmd);
}

int
vmcmd_readvn(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	struct exec_vmcmd *cmd = elp->el_vmcmds->evs_cmds;

	int error;

	error = vn_rdwr(UIO_READ, cmd->ev_handle, (caddr_t)cmd->ev_addr, cmd->ev_size, cmd->ev_offset, UIO_USERSPACE, IO_UNIT, p->p_ucred, NULL, p);
	if (error)
		return error;


	if(cmd->ev_prot != (VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE)) {
		return (vm_map_protect(&vmspace->vm_map, trunc_page(cmd->ev_addr), round_page(cmd->ev_addr + cmd->ev_size), cmd->ev_prot, FALSE));
	} else {
		return (KERN_SUCCESS);
	}
}

/*
 * vmcmd_map_zero():
 *	handle vmcmd which specifies a zero-filled address space region.  The
 *	address range must be first allocated, then protected appropriately.
 */
int
vmcmd_map_zero(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	struct exec_vmcmd *cmd = elp->el_vmcmds->evs_cmds;

	int error;
	long diff;

	if (cmd->ev_size == 0)
		return(KERN_SUCCESS); /* XXXCDC: should it happen? */

	diff = cmd->ev_addr - trunc_page(cmd->ev_addr);
	cmd->ev_addr -= diff;			/* required by uvm_map */
	cmd->ev_size += diff;

	error = vm_mmap(&vmspace->vm_map, &cmd->ev_addr, round_page(cmd->ev_size), VM_PROT_DEFAULT, cmd->ev_maxprot, cmd->ev_flags, NULL, cmd->ev_offset);
	if (error)
		return error;
	return (KERN_SUCCESS);
}

/*
 * Copy out argument and environment strings from the old process
 *	address space into the temporary string buffer.
 */
int
exec_extract_strings(elp, dp, cpp)
	struct exec_linker *elp;
	char *dp;
	char * const *cpp;
{
	char	**argv, **envv, **tmpfap;
	char	*argp, *envp;
	int		error, length;

#ifdef DIAGNOSTIC
		if (argp == (vaddr_t) 0)
			panic("execve: argp == NULL");
#endif
	dp = argp;
	if(elp->el_flags & EXEC_HASARGL) {
		tmpfap = elp->el_fa;
		while (*tmpfap != NULL) {
			char *cp;

			cp = *tmpfap;
			while (*cp)
				*dp++ = *cp++;
			dp++;

			FREE(*tmpfap, M_EXEC);
			tmpfap++; elp->el_argc++;
		}
		FREE(elp->el_fa, M_EXEC);
		elp->el_flags &= ~EXEC_HASARGL;
	}

	if(!(cpp = SCARG(elp->el_uap, elp->el_uap->argp))) {
		error = EINVAL;
		return error;
	}

	if(elp->el_flags & EXEC_SKIPARG)
		cpp++;
		continue;

	/* extract arguments first */
	argv = elp->el_uap->argp;
	if (argv) {
		while ((argp = (caddr_t) fuword(argv++))) {
			if (argp == (caddr_t) -1)
				return (EFAULT);
			if ((error = copyinstr(argp, elp->el_stringp, elp->el_stringspace, &length))) {
				if (error == ENAMETOOLONG)
					return (E2BIG);
				return (error);
			}
			elp->el_stringspace -= length;
			elp->el_stringp += length;
			elp->el_argc++;
		}
	}

	/* extract environment strings */
	envv = elp->el_uap->envp;
	if (envv) {
		while ((envp = (caddr_t) fuword(envv++))) {
			if (envp == (caddr_t) -1)
				return (EFAULT);
			if ((error = copyinstr(envp, elp->el_stringp, elp->el_stringspace, &length))) {
				if (error == ENAMETOOLONG)
					return(E2BIG);
				return (error);
			}
			elp->el_stringspace -= length;
			elp->el_stringp += length;
			elp->el_envc++;
		}
	}

	dp = (char *) ALIGN(dp);
	return (0);
}

/*
 * Copy strings out to the new process address space, constructing
 * new arg and env vector tables. Return a pointer to the base
 * so that it can be used as the initial stack pointer.
 */
int *
exec_copyout_strings(elp)
	struct exec_linker *elp;
{
	int argc, envc;
	char **vectp;
	char *stringp, *destp;
	int *stack_base;
	struct ps_strings *arginfo;
	char *dp, *sp;
	char **tmpfap;

	/* Calculate string base and vector table pointers. */
	arginfo = PS_STRINGS;
	destp =	(caddr_t)arginfo - roundup((ARG_MAX - elp->el_stringspace), sizeof(char *));

	/* The '+ 2' is for the null pointers at the end of each of the arg and	env vector sets */
	vectp = (char **) (destp - (elp->el_argc + elp->el_envc + 2) * sizeof(char *));

	/* vectp also becomes our initial stack base */
	stack_base = (int *)vectp;

	stringp = elp->el_stringbase;
	argc = elp->el_argc;
	envc = elp->el_envc;

	/* Fill in "ps_strings" struct for ps, w, etc. */
	arginfo->ps_argvstr = destp;
	arginfo->ps_nargvstr = argc;

	/* Copy the arg strings and fill in vector table as we go. */
	for (; argc > 0; --argc) {
		*(vectp++) = destp;
		while ((*destp++ = *stringp++));
	}

	/* a null vector table pointer seperates the argp's from the envp's */
	*(vectp++) = NULL;

	arginfo->ps_envstr = destp;
	arginfo->ps_nenvstr = envc;

	/* Copy the env strings and fill in vector table as we go. */
	for (; envc > 0; --envc) {
		*(vectp++) = destp;
		while ((*destp++ = *stringp++));
	}

	/* end of vector table is a null pointer */
	*vectp = NULL;

	return (stack_base);
}
