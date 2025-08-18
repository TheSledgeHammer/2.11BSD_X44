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
#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/filedesc.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/mman.h>
#include <sys/resourcevar.h>
#include <sys/user.h>
#include <sys/device.h>
#include <sys/signalvar.h>
#include <sys/acct.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/ktrace.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sysdecl.h>
#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>

#include <machine/cpu.h>
#include <machine/reg.h>

extern void syscall();
extern char	sigcode[], esigcode[];

static int doexecve(struct proc *, struct execa_args *, register_t *);

struct emul emul_211bsd = {
		.e_name 		= "211bsd",
		.e_path 		= NULL,				/* emulation path */
		.e_nsysent		= SYS_MAXSYSCALL,
		.e_sysent 		= sysent,
#ifdef SYSCALL_DEBUG
		.e_syscallnames = syscallnames,
#else
		.e_syscallnames = NULL,
#endif
		.e_arglen 		= 0,
		.e_sendsig		= sendsig,
		.e_setregs 		= setregs,
		.e_sigcode 		= sigcode,
		.e_esigcode 	= esigcode,
		.e_syscall		= syscall
};

/*
 * Execa: (on startup only)
 * Serves a special purpose during startup of loading
 * the execa_args from the user args pointer.
 */
int
execa(p, args, retval)
	struct proc *p;
	struct execa_args *args;
	register_t *retval;
{
	return (doexecve(p, args, retval));
}

/*
 * setup execa_args.
 */
void
doexeca(args, fname, argp, envp)
	struct execa_args 	*args;
	char				*fname;
	char				**argp;
	char				**envp;
{
	args = (struct execa_args *)u.u_ap;
	if (args != NULL) {
		SCARG(args, fname) = fname;
		SCARG(args, argp) = argp;
		SCARG(args, envp) = envp;
	}
}

int
execv()
{
	struct execa_args *uap = (struct execa_args *)u.u_ap;
	SCARG(uap, envp) = NULL;
	return (execve(SCARG(uap, fname), SCARG(uap, argp), NULL));
}

static int
doexecve(p, args, retval)
	struct proc *p;
	struct execa_args *args;
	register_t *retval;
{
	int error;

	if (args != NULL) {
		error = execve(SCARG(args, fname), SCARG(args, argp), SCARG(args, envp));
	} else {
		error = -1;
	}
	u.u_procp = p;
	u.u_error = error;
	u.u_r.r_val1 = retval[0];
	u.u_r.r_val2 = retval[1];
	return (error);
}

int
execve()
{
	struct proc *p;
	struct execa_args *uap;
	register_t *retval;
	struct nameidata ndp;
	struct ps_strings *arginfo;
	struct exec_linker elp;
	struct vmspace *vm;
	struct vattr attr;
	struct exec_vmcmd *base_vcp;
	int error, i, szsigcode;
	size_t len;
	char *stack, *stack_base;
	char *dp, *sp;
	char **tmpfap;

	uap = (struct execa_args *)u.u_ap;
	
	p = u.u_procp;
	p->p_flag |= P_INEXEC;
    base_vcp = NULL;
    
	if (exec_maxhdrsz == 0) {
		for (i = 0; i < nexecs; i++) {
			if (execsw[i].ex_makecmds != NULL
					&& execsw[i].ex_hdrsz > exec_maxhdrsz) {
				exec_maxhdrsz = execsw[i].ex_hdrsz;
			}
		}
	}
	
	NDINIT(&ndp, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, fname), p);
	
	/* Initialize a few constants in the common area */
	elp.el_name = SCARG(uap, fname);
	elp.el_image_hdr = malloc(exec_maxhdrsz, M_EXEC, M_WAITOK);
	elp.el_hdrlen = exec_maxhdrsz;
	elp.el_hdrvalid = 0;
	elp.el_proc = p;
	elp.el_uap = uap;
	elp.el_attr = &attr;
	elp.el_argc = 0;
	elp.el_envc = 0;
	elp.el_entry = 0;
	elp.el_ndp = ndp;
	elp.el_emul_arg = NULL;
	elp.el_vmcmds.evs_cnt = 0;
	elp.el_vmcmds.evs_used = 0;
	elp.el_flags = 0;

	/* Allocate temporary demand zeroed space for argument and environment strings */
	error = vm_allocate(kernel_map, (vm_offset_t*) &elp.el_stringbase, ARG_MAX, TRUE);

	if (error) {
		log(LOG_WARNING, "execve: failed to allocate string space\n");
		return (u.u_error);
	}

	if (!elp.el_stringbase) {
		error = ENOMEM;
		goto exec_abort;
	}
	elp.el_stringp = elp.el_stringbase;
	elp.el_stringspace = ARG_MAX;

	/* see if we can run it. */
	if ((error = check_exec(&elp)) != 0)
		goto freehdr;

	error = exec_extract_strings(&elp, dp);
	if (error != 0) {
		goto bad;
	}

	szsigcode = elp.el_es->ex_emul->e_esigcode - elp.el_es->ex_emul->e_sigcode;
	
	long argc = elp.el_argc;
	long envc = elp.el_envc;
	char *argp = *SCARG(uap, argp);
	
	/* Now check if args & environ fit into new stack */
	if (elp.el_flags & EXEC_32) {
		len = ((argc + envc + 2 + elp.el_es->ex_arglen)
				* sizeof(int) + sizeof(int) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - argp;
	} else {
		len = ((argc + envc + 2 + elp.el_es->ex_arglen)
				* sizeof(char *) + sizeof(int) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - argp;
	}

	len = ALIGN(len); /* make the stack "safely" aligned */

	if (len > elp.el_ssize) {
		error = ENOMEM;
		goto bad;
	}

	/* adjust "active stack depth" for process VSZ */
	elp.el_ssize = len;

	/* Map address Space  & create new process's VM space */
	error = vmcmd_create_vmspace(&elp);
	if (error != 0) {
		goto exec_abort;
	}

	stack_base = (char *)exec_copyout_strings(&elp, arginfo);
	stack = (char *)(vm->vm_minsaddr - len);
	if (stack == stack_base) {
		/* Now copy argc, args & environ to new stack */
		if (!(*elp.el_es->ex_copyargs)(&elp, arginfo, stack, SCARG(elp.el_uap, argp)))
			goto exec_abort;
	} else {
		/* Now copy argc, args & environ to stack_base */
		if (!(*elp.el_es->ex_copyargs)(&elp, arginfo, stack_base, SCARG(elp.el_uap, argp)))
			goto exec_abort;
	}

	/* copy out the process's ps_strings structure */
	if (copyout(&arginfo, (char*) PS_STRINGS, sizeof(arginfo)))
		goto exec_abort;
		
	stopprofclock(p);   /* stop profiling */
	fdcloseexec(); 	    /* handle close on exec */
	execsigs(p); 	    /* reset catched signals */

	p->p_ctxlink = NULL; /* reset ucontext link */

	/* set command name & other accounting info */
	len = min(ndp.ni_cnd.cn_namelen, MAXCOMLEN);
	memcpy(p->p_comm, ndp.ni_cnd.cn_nameptr, len);
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
		wakeup((caddr_t) p->p_pptr);
	}

	/* Turn off kernel tracing for set-id programs, except for root. */
#ifdef KTRACE
	if (p->p_tracep && !(p->p_traceflag & KTRFAC_ROOT) && (attr.va_mode & (VSUID | VSGID))
			&& suser1(p->p_ucred, &p->p_acflag)) {
	    ktrderef(p);
	}
#endif
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

	if (vm_deallocate(kernel_map, (vm_offset_t) elp.el_stringbase, ARG_MAX))
		panic("execve: string buffer dealloc failed (1)");
	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);
	vn_lock(elp.el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(elp.el_vnodep, FREAD, p->p_ucred, p);
	vput(elp.el_vnodep);

	/* setup new registers and do misc. setup. */
	if (stack == stack_base) {
		(*elp.el_es->ex_emul->e_setregs)(p, &elp, (u_long)stack);
	} else {
		(*elp.el_es->ex_emul->e_setregs)(p, &elp, (u_long)stack_base);
	}

	if (p->p_flag & P_TRACED)
		psignal(p, SIGTRAP);
	
	/* update p_emul, the old value is no longer needed */
	p->p_emul = elp.el_es->ex_emul;
	/* ...and the same for p_execsw */
	p->p_execsw = elp.el_es;
	FREE(elp.el_image_hdr, M_EXEC);

	p->p_flag &= ~P_INEXEC;
	return (EJUSTRETURN);

bad:
	p->p_flag &= ~P_INEXEC;
	/* free the vmspace-creation commands, and release their references */
	kill_vmcmd(&elp.el_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (elp.el_flags & EXEC_HASFD) {
		elp.el_flags &= ~EXEC_HASFD;
		(void) fdrelease(elp.el_fd);
	}
	/* close and put the exec'd file */
	vn_lock(elp.el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(elp.el_vnodep, FREAD, p->p_ucred, p);
	vput(elp.el_vnodep);
	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);

freehdr:
	p->p_flag &= ~P_INEXEC;
	FREE(elp.el_image_hdr, M_EXEC);
	return (error);

exec_abort:
	p->p_flag &= ~P_INEXEC;
	vm_deallocate(kernel_map, VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
	if (elp.el_emul_arg)
		FREE(elp.el_emul_arg, M_TEMP);
	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);
	vn_lock(elp.el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(elp.el_vnodep, FREAD, p->p_ucred, p);
	vput(elp.el_vnodep);
	FREE(elp.el_image_hdr, M_EXEC);
	exit(W_EXITCODE(0, SIGABRT));
	exit(-1);

	return (0);
}

/*
int
fexecve()
{
	register struct fexeca_args {
		syscallarg(int) 	fd;
		syscallarg(char	**) argp;
		syscallarg(char	**) envp;
	} *uap = (struct fexeca_args *)u.u_ap;

	return (0);
}
*/

/*
 * Reset signals for an exec of the specified process.  In 4.4 this function
 * was in kern_sig.c but since in 2.11 kern_sig and kern_exec will likely be
 * in different overlays placing this here potentially saves a kernel overlay
 * switch.
 */

extern char sigprop[NSIG + 1];

void
execsigs(p)
	register struct proc *p;
{
    register struct sigacts *ps;
	register int nc, mask;

    ps = p->p_sigacts;

	/*
	 * Reset caught signals.  Held signals remain held
	 * through p_sigmask (unless they were caught,
	 * and are now ignored by default).
	 */
	while (p->p_sigcatch) {
		nc = ffs((long)p->p_sigcatch);
		mask = sigmask(nc);
		p->p_sigcatch &= ~mask;
		if (sigprop[nc] & SA_IGNORE) {
			if (nc != SIGCONT) {
				p->p_sigignore |= mask;
			}
			p->p_sig &= ~mask;
		}
        u.u_signal[nc] = SIG_DFL;
	}
	/*
	 * Reset stack state to the user stack (disable the alternate stack).
	 */
	ps->ps_sigstk = u.u_sigstk;
	u.u_sigstk.ss_flags = SA_DISABLE;
	u.u_sigstk.ss_size = 0;
	u.u_sigstk.ss_base = 0;
	u.u_psflags = 0;
}

int
check_exec(elp)
	struct exec_linker *elp;
{
	int					error, i;
	struct vnode		*vp;
	struct nameidata 	*ndp;
	size_t				resid;
	struct proc 		*p;

	p = elp->el_proc;
	ndp = &elp->el_ndp;
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
	if ((error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p)) != 0)
		goto bad1;

	/* get attributes */
	if ((error = VOP_GETATTR(vp, elp->el_attr, p->p_ucred, p)) != 0)
		goto bad1;

	/* Check mount point */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto bad1;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID)
		elp->el_attr->va_mode &= ~(S_ISUID | S_ISGID);

	/* try to open it */
	if ((error = VOP_OPEN(vp, FREAD, p->p_ucred, p)) != 0)
		goto bad1;

	/* unlock vp, since we need it unlocked from here on out. */
	VOP_UNLOCK(vp, 0, p);

	error = vn_rdwr(UIO_READ, vp, elp->el_image_hdr, elp->el_hdrlen, 0,
				UIO_SYSSPACE, 0, p->p_ucred, &resid, NULL);

	if (error)
		goto bad2;
	elp->el_hdrvalid = elp->el_hdrlen - resid;

	/*
	 * Set up default address space limits.  Can be overridden
	 * by individual exec packages.
	 *
	 * XXX probably should be all done in the exec pakages.
	 */
	elp->el_vm_minaddr = (caddr_t)VM_MIN_ADDRESS;
	elp->el_vm_maxaddr = (caddr_t)VM_MAXUSER_ADDRESS;
	/*
	 * set up the vmcmds for creation of the process
	 * address space
	 */
	error = ENOEXEC;
	for (i = 0; i < nexecs && error != 0; i++) {
		int newerror;

		elp->el_esch = &execsw[i];
		newerror = (*execsw[i].ex_makecmds)(elp);
		/* make sure the first "interesting" error code is saved. */
		if (!newerror || error == ENOEXEC)
			error = newerror;

		/* if es_makecmds call was successful, update epp->ep_es */
		if (!newerror && (elp->el_flags & EXEC_HASES) == 0)
			elp->el_es = &execsw[i];

		if ((elp->el_flags & EXEC_DESTR) && error != 0)
			return error;
	}
	if (!error) {
		/* check that entry point is sane */
		if (elp->el_entry > VM_MAXUSER_ADDRESS)
			error = ENOEXEC;

		/* check limits */
		if ((elp->el_tsize > MAXTSIZ) ||
				(elp->el_dsize > (u_quad_t)u.u_rlimit[RLIMIT_DATA].rlim_cur))
			error = ENOMEM;

		if (!error)
			return (0);
	}
	/*
	 * free any vmspace-creation commands,
	 * and release their references
	 */
	kill_vmcmd(&elp->el_vmcmds);

bad2:
	/*
	 * close and release the vnode, restore the old one, free the
	 * pathname buf, and punt.
	 */
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(vp, FREAD, p->p_ucred, p);
	vput(vp);
	return (error);

bad1:
	/*
	 * free the namei pathname buffer, and put the vnode
	 * (which we don't yet have open).
	 */
	vput(vp);				/* was still locked */
	return (error);
}

int
copyargs(elp, arginfo, stack, argp)
	struct exec_linker *elp;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	char **cpp;
	char *dp, *sp;
	size_t len;
	void *nullp;
	long argc, envc;
	int error;

	cpp = (char **)stack;
	nullp = NULL;
	argc = arginfo->ps_nargvstr;
	envc = arginfo->ps_nenvstr;

	dp = (char *)(cpp +  1 + argc + 1+ envc) + elp->el_es->ex_emul->e_arglen;
	sp = argp;

	if ((error = copyout(&argc, cpp++, sizeof(argc))) != 0) {
		return (error);
	}

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = *cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len) {
		if (copyout(&dp, cpp++, sizeof(dp)) != 0) {
			return (error);
		}
		if (copyoutstr(sp, dp, ARG_MAX, &len) != 0) {
			return (error);
		}
	}

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0) {
		return (error);
	}

	arginfo->ps_envstr = *cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len) {
		if (copyout(&dp, cpp++, sizeof(dp)) != 0) {
			return (error);
		}
		if (copyoutstr(sp, dp, ARG_MAX, &len) != 0) {
			return (error);
		}
	}

	if ((error = copyout(&nullp, cpp++, sizeof(nullp))) != 0) {
		return (error);
	}

    stack = (char *)cpp;
	return (0);
}
