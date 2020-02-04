/*	$NetBSD: kern_exec.c,v 1.110.4.8 2002/04/26 17:51:39 he Exp $	*/
/*-
 * Copyright (C) 1993, 1994, 1996 Christopher G. Demetriou
 * Copyright (C) 1992 Wolfgang Solfrank.
 * Copyright (C) 1992 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/resourcevar.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/signalvar.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
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
		char *stack, *stack_base;
		struct ps_strings *arginfo;
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

		/* Allocate temporary demand zeroed space for argument and environment strings */
		error = vm_allocate(kernel_map, (vm_offset_t *)&elp->el_stringbase, ARG_MAX, TRUE);

		if(error) {
			log(LOG_WARNING, "execve: failed to allocate string space\n");
			return (u->u_error);
		}

		if (!elp->el_stringbase) {
			error = ENOMEM;
				goto exec_abort;
		}
		elp->el_stringp = elp->el_stringbase;
		elp->el_stringspace = ARG_MAX;

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

		/* From to line 149 may change */
		stack_base = (char *) exec_copyout_strings(elp, arginfo);
		stack = (char *) (vm->vm_minsaddr - len);
		if(stack == stack_base) {
			/* Now copy argc, args & environ to new stack */
			if(!(*elp.el_emul->e_copyargs)(&elp, &arginfo, stack, elp.el_uap->argp))
				goto exec_abort;
		} else {
			/* Now copy argc, args & environ to stack_base */
			if(!(*elp.el_emul->e_copyargs)(&elp, &arginfo, stack_base, elp.el_uap->argp))
				goto exec_abort;
		}

		/* copy out the process's ps_strings structure */
		if (copyout(&arginfo, (char *) PS_STRINGS, sizeof(arginfo)))
			goto exec_abort;

		fdcloseexec(p);		/* handle close on exec */
		execsigs(p);		/* reset catched signals */

		/* set command name & other accounting info */
		len = min(ndp->ni_cnd.cn_namelen, MAXCOMLEN);
		memcpy(p->p_comm, ndp->ni_cnd.cn_nameptr, len);
		p->p_comm[len] = 0;
		p->p_acflag &= ~AFORK;

		/* record proc's vnode, for use by procfs and others */
		if (p->p_textvp)
			vrele(p->p_textvp);
		VREF(elp.el_vnodep);
		p->p_textvp = elp.el_vnodep;

		p->p_flag |= P_EXEC;
		if (p->p_pptr && (p->p_flag & P_PPWAIT)) {
			p->p_flag &= ~P_PPWAIT;
			wakeup((caddr_t)p->p_pptr);
		}

		/* FreeBSD: Turn off kernel tracing for set-id programs, except for root. */
		if (p->p_tracep && (attr.va_mode & (VSUID | VSGID)) && suser(p->p_ucred, &p->p_acflag)) {
			p->p_traceflag = 0;
			vrele(p->p_tracep);
			p->p_tracep = 0;
		}
		if ((attr.va_mode & VSUID) && (p->p_flag & P_TRACED) == 0) {
			p->p_ucred = crcopy(p->p_ucred);
			p->p_ucred->cr_uid = attr.va_uid;
			p->p_flag |= P_SUGID;
		}
		if ((attr.va_mode & VSGID) && (p->p_flag & P_TRACED) == 0) {
			p->p_ucred = crcopy(p->p_ucred);
			p->p_ucred->cr_groups[0] = attr.va_gid;
			p->p_flag |= P_SUGID;
		}

		p->p_cred->p_svuid = p->p_ucred->cr_uid;
		p->p_cred->p_svgid = p->p_ucred->cr_gid;

		if (vm_deallocate(kernel_map, (vm_offset_t)elp->el_stringbase, ARG_MAX))
			panic("execve: string buffer dealloc failed (1)");
		FREE(ndp->ni_cnd.cn_pnbuf, M_NAMEI);
		vn_lock(elp.el_vnodep, LK_EXCLUSIVE | LK_RETRY);
		VOP_CLOSE(elp.el_vnodep, FREAD, p->p_cred, p);
		vput(elp.el_vnodep);

		/* setup new registers and do misc. setup. */
		(*elp.el_emul->e_setregs)(p, &elp, (u_long) stack);

		if (p->p_flag & P_TRACED)
				psignal(p, SIGTRAP);

		p->p_emul = elp.el_emul;
		FREE(elp.el_image_hdr, M_EXEC);

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
		VOP_CLOSE(elp.el_vnodep, FREAD, p->p_cred, p);
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
		exit1(p, W_EXITCODE(0, SIGABRT));
		exit1(p,-1);

		return (0);
}

/*
 * Reset signals for an exec of the specified process.  In 4.4 this function
 * was in kern_sig.c but since in 2.11 kern_sig and kern_exec will likely be
 * in different overlays placing this here potentially saves a kernel overlay
 * switch.
 */
void
execsigs(p)
	register struct proc *p;
{
	register int nc;
	unsigned long mask;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	while (p->p_sigcatch) {
		nc = ffs(p->p_sigcatch);
		mask = sigmask(nc);
		p->p_sigcatch &= ~mask;
		if (sigprop[nc] & SA_IGNORE) {
			if (nc != SIGCONT)
				p->p_sigignore |= mask;
			p->p_sigacts &= ~mask;
		}
		u->u_signal[nc] = SIG_DFL;
	}
	/*
	 * Reset stack state to the user stack (disable the alternate stack).
	 */
	u->u_sigstk.ss_flags = SA_DISABLE;
	u->u_sigstk.ss_size = 0;
	u->u_sigstk.ss_base = 0;
	u->u_psflags = 0;
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

void *
copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	char **cpp = stack;
	char *dp, *sp;
	size_t len;
	void *nullp = NULL;
	long argc = arginfo->ps_nargvstr;
	long envc = arginfo->ps_nenvstr;

	dp = (char *) (cpp + argc + envc + 2 + pack->ep_emul->e_arglen);
	sp = argp;

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	arginfo->ps_envstr = cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	return cpp;
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
