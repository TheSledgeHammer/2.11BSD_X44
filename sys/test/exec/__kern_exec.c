/*
 * __kern_exec.c
 *
 *  Created on: 31 Jan 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <exec/__exec.h>
#include <exec/__exec_linker.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <vm/include/vm_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>

extern const struct execsw	execsw_builtin[];
extern int			nexecs_builtin;
static const struct execsw	**execsw = NULL;
static int			nexecs;

static int check_exec(struct exec_linker *);
static void link_ex(struct execsw_entry **listp, const struct execsw *exp);

void
execv(p, uap, retval)
	struct proc *p;
	struct execa *uap = (struct execa *)u->u_ap;
	int *retval;
{
	uap->envp = NULL;
	execve(p, uap, retval);
}

int
execve(p, uap, retval)
	struct proc *p;
	register struct execa *uap = (struct execa *)u->u_ap;
	int *retval;
{
		struct nameidata nd, *ndp;
		int error, i, szsigcode, len;
		struct exec_linker elp;
		struct vmspace *vm;
		struct vnode *vnodep;
		struct vattr attr;
		char *dp, *sp;
		char * const *cpp;
		char **tmpfap;
		struct exec_vmcmd *base_vcp = NULL;
		extern struct emul emul_211bsd;

		if (exec_maxhdrsz == 0) {
			for (i = 0; i < nexecs; i++)
				if (execsw[i]->ex_check != NULL && execsw[i]->ex_hdrsz > exec_maxhdrsz)
					exec_maxhdrsz = execsw[i]->ex_hdrsz;
		}

		/* Initialize a few constants in the common area */
		MALLOC(elp, struct exec_linker *, sizeof(elp->el_image_hdr), M_EXEC, M_WAITOK);
		elp->el_proc = p;
		elp->el_uap = uap;
		elp->el_attr = &attr;
		elp->el_argc = elp->el_envc = 0;
		elp->el_entry = 0;
		elp->el_hdrlen = exec_maxhdrsz;
		elp->el_hdrvalid = 0;
		elp->el_ndp = &ndp;
		elp->el_emul_arg = NULL;
		elp->el_vmcmds->evs_cnt = 0;
		elp->el_vmcmds->evs_used = 0;
		elp->el_vnodep = NULL;
		elp->el_emul = &emul_211bsd;
		elp->el_flags = 0;


		/* see if we can run it. */
		if ((error = check_exec(p, &elp)) != 0)
			goto freehdr;

		error = exec_extract_strings(elp, dp, cpp);
		if(error != 0) {
			goto bad;
		}

		szsigcode = elp.el_emul->e_esigcode - elp.el_emul->e_sigcode;

		/* Now check if args & environ fit into new stack */
		if(elp->el_flags & EXEC_32)
			len = ((elp.el_argc + elp->el_envc + 2 + elp.el_emul->e_arglen) * sizeof(int) +
				       sizeof(int) + dp + STACKGAPLEN + szsigcode +
				       sizeof(struct ps_strings)) - uap->argp;
		else
			len = ((elp.el_argc + elp->el_envc+ 2 + elp.el_emul->e_arglen) * sizeof(char *) +
				       sizeof(int) + dp + STACKGAPLEN + szsigcode +
				       sizeof(struct ps_strings)) - uap->argp;

		len = ALIGN(len);	/* make the stack "safely" aligned */

		if(len > elp.el_ssize)
			error = ENOMEM;
			goto bad;

		/* adjust "active stack depth" for process VSZ */
		elp.el_ssize = len;

		/* Map address Space  & create new process's VM space */
		error = vmcmd_create_vmspace(elp);
		if(error != 0) {
			goto exec_abort;
		}


		p->p_flag &= ~P_INEXEC;
			return (EJUSTRETURN);
bad:
		p->p_flag &= ~P_INEXEC;
		/* free the vmspace-creation commands, and release their references */
		kill_vmcmds(&elp.el_vmcmds);
		/* kill any opened file descriptor, if necessary */
		if (elp.el_flags & EXEC_HASFD) {
			elp.el_flags &= ~EXEC_HASFD;
			(void) fdrelease(p, elp.el_fd);
		}
		/* close and put the exec'd file */
		vn_lock(elp.el_vnodep, LK_EXCLUSIVE | LK_RETRY);
		VOP_CLOSE(elp.el_vnodep, FREAD, cred, p);
		vput(elp.el_vnodep);
		FREE(ndp->ni_cnd.cn_pnbuf, M_NAMEI);

freehdr:
		p->p_flag &= ~P_INEXEC;
		FREE(elp.el_image_hdr, M_EXEC);
		return error;

exec_abort:
		p->p_flag &= ~P_INEXEC;
		vm_deallocate(&vm, VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
		if(elp->el_emul_arg)
			FREE(elp->el_emul_arg, M_TEMP);
		FREE(ndp->ni_cnd.cn_pnbuf, M_NAMEI);
		vn_lock(elp->el_vnodep, LK_EXCLUSIVE | LK_RETRY);
		VOP_CLOSE(elp->el_vnodep, FREAD, p->p_cred, p);
		vput(elp->el_vnodep);
		FREE(elp->el_image_hdr, M_EXEC);

		return (0);
}

int
check_exec(elp)
	struct exec_linker *elp;
{
	int					error, i;
	struct vnode		*vp;
	struct nameidata 	*ndp;
	size_t				resid;


	struct proc *p = elp->el_proc;
	ndp = elp->el_ndp;

	ndp->ni_cnd.cn_nameiop = LOOKUP;
	ndp->ni_cnd.cn_flags = FOLLOW | LOCKLEAF | SAVENAME;
	/* first get the vnode */
	if ((error = namei(ndp)) != 0)
		return error;
	elp->el_vnodep = vp = ndp->ni_vp;

	/* check access and type */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto bad1;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, p->p_cred, p)) != 0)
		goto bad1;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, elp->el_vnodep, p->p_cred, p)) != 0)
		goto bad1;

	/* Check mount point */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto bad1;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID)
		elp->el_attr->va_mode &= ~(S_ISUID | S_ISGID);

	/* try to open it */
	if ((error = VOP_OPEN(vp, FREAD, p->p_cred, p)) != 0)
		goto bad1;

	/* unlock vp, since we need it unlocked from here on out. */
	VOP_UNLOCK(vp, 0);

	error = vn_rdwr(UIO_READ, vp, elp->el_image_hdr, elp->el_hdrlen, 0,
				UIO_SYSSPACE, 0, p->p_cred, &resid, NULL);

	if (error)
		goto bad2;
	elp->el_hdrvalid = elp->el_hdrlen - resid;

	/*
	 * Set up default address space limits.  Can be overridden
	 * by individual exec packages.
	 *
	 * XXX probably should be all done in the exec pakages.
	 */
	elp->el_vm_minaddr = VM_MIN_ADDRESS;
	elp->el_vm_maxaddr = VM_MAXUSER_ADDRESS;
	/*
	 * set up the vmcmds for creation of the process
	 * address space
	 */
	error = ENOEXEC;
	for (i = 0; i < nexecs && error != 0; i++) {
		int newerror;

		elp->el_esch = execsw[i];
		newerror = (*execsw[i]->ex_check)(p, elp);
		/* make sure the first "interesting" error code is saved. */
		if (!newerror || error == ENOEXEC)
			error = newerror;

		/* if es_makecmds call was successful, update epp->ep_es */
		if (!newerror && (elp->el_flags & EXEC_HASES) == 0)
			elp->el_esch = execsw[i];

		if ((elp->el_flags & EXEC_DESTR) && error != 0)
			return error;
	}
	if (!error) {
		/* check that entry point is sane */
		if (elp->el_entry > VM_MAXUSER_ADDRESS)
			error = ENOEXEC;

		/* check limits */
		if ((elp->el_tsize > MAXTSIZ) ||
				(elp->el_dsize > (u_quad_t)p->p_rlimit[RLIMIT_DATA].rlim_cur))
			error = ENOMEM;

		if (!error)
			return (0);
	}
	/*
	 * free any vmspace-creation commands,
	 * and release their references
	 */
	kill_vmcmds(&elp->el_vmcmds);

bad2:
	/*
	 * close and release the vnode, restore the old one, free the
	 * pathname buf, and punt.
	 */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	VOP_CLOSE(vp, FREAD, p->p_cred, p);
	vput(vp);
	return error;

bad1:
	/*
	 * free the namei pathname buffer, and put the vnode
	 * (which we don't yet have open).
	 */
	vput(vp);				/* was still locked */
	return error;
}

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
	struct proc *p = elp->el_proc;
	struct vmspace *vmspace = p->p_vmspace;
	struct exec_vmcmd *base_vcp = NULL;

	int error, i;

	/*  Now map address space */
	vmspace->vm_tsize = btoc(elp->el_tsize);
	vmspace->vm_dsize = btoc(elp->el_dsize);
	vmspace->vm_taddr = (caddr_t) elp->el_taddr;
	vmspace->vm_daddr = (caddr_t) elp->el_daddr;
	vmspace->vm_ssize = btoc(elp->el_ssize);
	vmspace->vm_minsaddr = (caddr_t)elp->el_minsaddr;
	vmspace->vm_maxsaddr = (caddr_t)elp->el_maxsaddr;

	/* create the new process's VM space by running the vmcmds */
#ifdef DIAGNOSTIC
	if(elp->el_vmcmds->evs_used == 0)
		panic("execve: no vmcmds");
#endif
	for (i = 0; i < elp->el_vmcmds->evs_used && !error; i++) {
		struct exec_vmcmd *vcp;

		vcp = elp->el_vmcmds->evs_cmds[i];
		if(vcp->ev_flags & VMCMD_RELATIVE) {
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
	kill_vmcmds(&elp->el_vmcmds);
	if(error) {
		return error;
	} else {
		return (0);
	}
}


/*
 * Add execsw[] entry.
 */
int
exec_add(struct proc *p, struct execsw *exp, const char *e_name)
{
	struct exec_entry	*it;
	int					error;

	error = 0;
	//lockmgr(&exec_lock, LK_EXCLUSIVE, NULL, p);

	LIST_FOREACH(it, &ex_head, ex_list) {
		if(it->ex->ex_check == exp->ex_check && it->ex->u.elf_probe_func == exp->u.elf_probe_func) {
			error = EEXIST;
			return error;
		}
	}

	/* if we got here, the entry doesn't exist yet */

	it = MALLOC(it, struct exec_entry *, sizeof(struct exec_entry), M_EXEC, M_WAITOK);
	it->ex = exp;
	LIST_INSERT_HEAD(&ex_head, it, ex_list);

	exec_init(0);

//out:
	//lockmgr(&exec_lock, LK_RELEASE, NULL, p);
	return error;
}

/*
 * Remove execsw[] entry.
 */
int
exec_remove(const struct execsw *exp)
{
	struct exec_entry	*it;
	int					error;

	error = 0;
	//lockmgr(&exec_lock, LK_EXCLUSIVE, NULL);

	LIST_FOREACH(it, &ex_head, ex_list) {
		if(it->ex->ex_check == exp->ex_check && it->ex->u.elf_probe_func == exp->u.elf_probe_func) {
			break;
		}
	}
	if (!it) {
		error = ENOENT;
		return error;
	}

	/* remove item from list and free resources */
	LIST_REMOVE(it, ex_list);
	FREE(it, M_EXEC);

	/* update execsw[] */
	exec_init(0);

//out:
	//lockmgr(&exec_lock, LK_RELEASE, NULL);
	return error;
}

static void
link_ex(struct execsw_entry **listp, const struct execsw *exp)
{
	struct execsw_entry *et, *e1;

	et = (struct execsw_entry *) malloc(sizeof(struct execsw_entry), M_TEMP, M_WAITOK);
	et->next = NULL;
	et->ex = exp;
	if (*listp == NULL) {
		*listp = et;
		return;
	}
	switch(et->ex->ex_prio) {
	case EXECSW_PRIO_FIRST:
		/* put new entry as the first */
		et->next = *listp;
		*listp = et;
		break;
	case EXECSW_PRIO_ANY:
		/* put new entry after all *_FIRST and *_ANY entries */
		for(e1 = *listp; e1->next
			&& e1->next->ex->ex_prio != EXECSW_PRIO_LAST;
			e1 = e1->next);
		et->next = e1->next;
		e1->next = et;
		break;
	case EXECSW_PRIO_LAST:
		/* put new entry as the last one */
		for(e1 = *listp; e1->next; e1 = e1->next);
		e1->next = et;
		break;
	default:
#ifdef DIAGNOSTIC
		panic("execw[] entry with unknown priority %d found",
			et->ex->ex_prio);
#else
		free(et, M_TEMP);
#endif
		break;
	}
}

int
exec_init(int init_boot)
{
	const struct execsw	**new_ex, * const *old_ex;
	struct execsw_entry	*list, *e1;
	struct exec_entry	*e2;
	int					i, ex_sz;

	if (init_boot) {
#ifdef DIAGNOSTIC
		if (i == 0)
			panic("no emulations found in execsw_builtin[]");
#endif
	}
	/*
	 * Build execsw[] array from builtin entries and entries added
	 * at runtime.
	 */
	list = NULL;
	for(i=0; i < nexecs_builtin; i++)
		link_es(&list, &execsw_builtin[i]);

	/* Add dynamically loaded entries */
	ex_sz = nexecs_builtin;
	LIST_FOREACH(e2, &ex_head, ex_list) {
		link_es(&list, e2->ex);
		ex_sz++;
	}

	/*
	 * Now that we have sorted all execw entries, create new execsw[]
	 * and free no longer needed memory in the process.
	 */
	new_ex = malloc(ex_sz * sizeof(struct execsw *), M_EXEC, M_WAITOK);
	for(i=0; list; i++) {
		new_ex[i] = list->ex;
		e1 = list->next;
		free(list, M_TEMP);
		list = e1;
	}

	/*
	 * New execsw[] array built, now replace old execsw[] and free
	 * used memory.
	 */
	old_ex = execsw;
	execsw = new_ex;
	nexecs = ex_sz;
	if (old_ex)
		free(old_ex, M_EXEC);

	/*
	 * Figure out the maximum size of an exec header.
	 */
	exec_maxhdrsz = 0;
	for (i = 0; i < nexecs; i++) {
		if (execsw[i]->ex_hdrsz > exec_maxhdrsz)
			exec_maxhdrsz = execsw[i]->ex_hdrsz;
	}
	return 0;
}
