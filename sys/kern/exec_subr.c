/*
 * exec_subr.c
 *
 *  Created on: 30 Nov 2019
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/mman.h>

#include <vm/include/vm.h>

int *exec_copyout_strings (struct exec_linker *);


/* Setup exec_mmap from exec_linker params */
void
exec_mmap_setup(elp, addr, size, prot, maxprot, flags, handle, offset)
	struct exec_linker *elp;
	vm_offset_t *addr;
	vm_size_t size;
	vm_prot_t prot, maxprot;
	int flags;
	caddr_t handle;
	unsigned long offset;
{
	struct exec_mmap *ex_map = (struct exec_mmap *) &elp;
	//setup with malloc();

	ex_map->em_addr = addr;
	ex_map->em_size = size;
	ex_map->em_prot = prot;
	ex_map->em_maxprot = maxprot;
	ex_map->em_flags = flags;
	ex_map->em_handle = handle;
	ex_map->em_offset = offset;

	elp->el_ex_map = ex_map; /* copy exec_linker map to exec_mmap */
}

/* Push exec_mmap into vmspace processing VM information and image params */
int
exec_mmap_to_vmspace(elp, entry)
	struct exec_linker *elp;
	u_long entry;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	struct exec_mmap *ex_map = elp->el_ex_map;
	int error;
	extern struct sysentvec sysvec;

	/* copy in arguments and/or environment from old process */
	error = exec_extract_strings(elp);
	if(error) {
		return (error);
	}

	/* Destroy old process VM and create a new one (with a new stack) */
	exec_new_vmspace(elp);

	error = vm_mmap(&vmspace->vm_map, ex_map->em_addr, ex_map->em_prot, ex_map->em_maxprot, ex_map->em_flags,
			ex_map->em_handle, ex_map->em_offset);
	if (error) {
		return (error);
	}

	error = vm_allocate(&vmspace->vm_map, ex_map->em_addr, ex_map->em_size, FALSE);
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

/*
 * Copy out argument and environment strings from the old process
 *	address space into the temporary string buffer.
 */
int
exec_extract_strings(elp)
	struct exec_linker *elp;
{
	char	**argv, **envv;
	char	*argp, *envp;
	int		error, length;

	/* extract arguments first */
	argv = elp->el_uap->argp;
	if (argv) {
		while ((argp = (caddr_t) fuword(argv++))) {
			if (argp == (caddr_t) -1)
				return (EFAULT);
			if ((error = copyinstr(argp, elp->el_stringp, elp->el_stringspace, &length))) {
				if (error == ENAMETOOLONG)
					return(E2BIG);
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

int
exec_setup_stack(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;

	u_long max_stack_size;
	u_long access_linear_min, access_size;
	u_long noaccess_linear_min, noaccess_size;

	max_stack_size = MAXSSIZ;
	elp->el_minsaddr = USRSTACK;
	elp->el_maxsaddr = elp->el_minsaddr + max_stack_size;

	elp->el_ssize = u->u_rlimit[RLIMIT_STACK].rlim_cur;

	access_size = elp->el_ssize;
	access_linear_min = (u_long)(elp->el_minsaddr - access_size);
	noaccess_size = max_stack_size - access_size;
	noaccess_linear_min = (u_long)((elp->el_minsaddr - access_size) + noaccess_size);
	noaccess_size = max_stack_size - access_size;

	if (noaccess_size > 0) {
		exec_mmap_setup(&vmspace->vm_map, elp->el_maxsaddr, ((elp->el_minsaddr - elp->el_ssize) - elp->el_maxsaddr),
				VM_PROT_NONE, VM_PROT_NONE, MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);
	}

	exec_mmap_setup(&vmspace->vm_map, elp->el_ssize, (elp->el_minsaddr - elp->el_ssize),
			VM_PROT_READ | VM_PROT_EXECUTE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE,
			MAP_PRIVATE | MAP_FIXED, (caddr_t)elp->el_vnodep, 0);

	return (0);
}
