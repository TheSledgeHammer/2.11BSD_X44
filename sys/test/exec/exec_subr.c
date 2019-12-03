/*
 * exec_subr.c
 *
 *  Created on: 30 Nov 2019
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/exec.h>

#include <sys/mman.h>

#include <vm/vm.h>

int *exec_copyout_strings __P((struct exec_linker *));

static int exec_check_permissions(struct exec_linker *);


/* Does this need to store more than one? If Yes: NetBSD exec_subr.c new_vmcmd */
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

		ex_map->em_addr = addr;
		ex_map->em_size = size;
		ex_map->em_prot = prot;
		ex_map->em_maxprot = maxprot;
		ex_map->em_flags = flags;
		ex_map->em_handle = handle;
		ex_map->em_offset = offset;

		elp->el_ex_map = ex_map;
}

int
exec_mmap_to_vmspace(elp)
	struct exec_linker *elp;
{
	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	struct exec_mmap *ex_map = elp->el_ex_map;
	int error;
	extern struct sysentvec sysvec;

	/* copy in arguments and/or environment from old process */
	error = exec_extract_strings(elp);
	if(error) {
		return error;
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
	vmspace->vm_tsize = elp->el_tsize;
	vmspace->vm_dsize = elp->el_dsize;
	vmspace->vm_taddr = elp->el_taddr;
	vmspace->vm_daddr = elp->el_daddr;

	/* Fill in image_params */
	elp->el_interpreted = 0;
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

	vmspace->vm_ssize = SGROWSIZ >> PAGE_SHIFT;

	/* Initialize maximum stack address */
	vmspace->vm_maxsaddr = (char *)USRSTACK - MAXSSIZ;

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

/*
 * Check permissions of file to execute.
 *	Return 0 for success or error code on failure.
 */
static int
exec_check_permissions(elp)
	struct exec_linker *elp;
{
	struct proc *p = elp->el_proc;
	struct vnode *vnodep = elp->el_vnodep;
	struct vattr *attr = elp->el_attr;
	int error;

	/* Check number of open-for-writes on the file and deny execution if there are any. */
	if (vnodep->v_writecount) {
		return (ETXTBSY);
	}

	/* Get file attributes */
	error = VOP_GETATTR(vnodep, attr, p->p_ucred, p);
	if (error)
		return (error);

	/*
	 * 1) Check if file execution is disabled for the filesystem that this
	 *	file resides on.
	 * 2) Insure that at least one execute bit is on - otherwise root
	 *	will always succeed, and we don't want to happen unless the
	 *	file really is executable.
	 * 3) Insure that the file is a regular file.
	 */
	if ((vnodep->v_mount->mnt_flag & MNT_NOEXEC) || ((attr->va_mode & 0111) == 0) || (attr->va_type != VREG)) {
		return (EACCES);
	}

	/* Zero length files can't be exec'd */
	if (attr->va_size == 0)
		return (ENOEXEC);
	/*
	 * Disable setuid/setgid if the filesystem prohibits it or if
	 *	the process is being traced.
	 */
	if ((vnodep->v_mount->mnt_flag & MNT_NOSUID) || (p->p_flag & P_TRACED))
		attr->va_mode &= ~(VSUID | VSGID);

	/*
	 *  Check for execute permission to file based on current credentials.
	 *	Then call filesystem specific open routine (which does nothing
	 *	in the general case).
	 */
	error = VOP_ACCESS(vnodep, VEXEC, p->p_ucred, p);
	if (error)
		return (error);

	error = VOP_OPEN(vnodep, FREAD, p->p_ucred, p);
	if (error)
		return (error);

	return (0);
}
