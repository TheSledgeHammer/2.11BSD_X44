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

static int 	doexecve(struct execa_args *);
static int 	execve_load(struct proc *, struct execa_args *, register_t *);

static int 	exec_create_vmspace(struct proc *, struct exec_linker *, struct exec_vmcmd *);
static char	*exec_extract_strings(struct exec_linker *, char **, char **, int, int *);
static char 	*exec_copyout_strings(struct exec_linker *, struct ps_strings *, struct vmspace *, int, int, int *);

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
execa(args)
	struct execa_args *args;
{
	return (doexecve(args));
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
doexecve(args)
	struct execa_args *args;
{
	int error;

	if (args != NULL) {
		error = execve(SCARG(args, fname), SCARG(args, argp), SCARG(args, envp));
	} else {
		error = -1;
	}
	u.u_error = error;
	return (error);
}

int
execve()
{
	struct proc *p;
	struct execa_args *uap;
	register_t *retval;

	p = u.u_procp;
	uap = (struct execa_args *)u.u_ap;
	return (execve_load(p, uap, retval));
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

static int
execve_load(p, uap, retval)
	struct proc *p;
	struct execa_args *uap;
	register_t *retval;
{
	struct exec_linker pack;
	struct nameidata ndp;
	struct ps_strings arginfo;
	struct vmspace	*vm;
	struct vattr attr;
	struct exec_vmcmd *base_vcp;
	int i, error, szsigcode;
	size_t len;
	char *stack_base, *dp;

	vm = p->p_vmspace;
	p->p_flag |= P_INEXEC;
	base_vcp = NULL;

	/*
	 * figure out the maximum size of an exec header, if necessary.
	 * XXX should be able to keep LKM code from modifying exec switch
	 * when we're still using it, but...
	 */
	if (exec_maxhdrsz == 0) {
		for (i = 0; i < nexecs; i++) {
			if (execsw[i].ex_makecmds != NULL
					&& execsw[i].ex_hdrsz > exec_maxhdrsz) {
				exec_maxhdrsz = execsw[i].ex_hdrsz;
			}
		}
	}

	/* init the namei data to point the file user's program name */
	/* XXX cgd 960926: why do this here?  most will be clobbered. */
	NDINIT(&ndp, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, fname), p);

	/* Initialize a few constants in the common area */
	pack.el_name = SCARG(uap, fname);
	MALLOC(pack.el_image_hdr, void *, exec_maxhdrsz, M_EXEC, M_WAITOK);
	pack.el_hdrlen = exec_maxhdrsz;
	pack.el_hdrvalid = 0;
	pack.el_attr = &attr;
	pack.el_argc = 0;
	pack.el_envc = 0;
	pack.el_entry = 0;
	pack.el_ndp = ndp;
	pack.el_emul_arg = NULL;
	pack.el_vmcmds.evs_cnt = 0;
	pack.el_vmcmds.evs_used = 0;
	pack.el_flags = 0;

	/* see if we can run it. */
	if ((error = check_exec(p, &pack)) != 0) {
		goto freehdr;
	}

	/* allocate an argument buffer */
	pack.el_stringbase = (char *)kmem_alloc_wait(exec_map, exec_maxhdrsz);
	if (pack.el_stringbase == NULL) {
		error = ENOMEM;
		goto exec_abort;
	}

	dp = exec_extract_strings(&pack, SCARG(uap, argp), SCARG(uap, envp), len, &error);
	if ((dp == NULL) && (error != 0)) {
		goto bad;
	}

	szsigcode = pack.el_es->ex_emul->e_esigcode - pack.el_es->ex_emul->e_sigcode;

	if (pack.el_flags & EXEC_32) {
		len = ((pack.el_argc + pack.el_envc + 2 + pack.el_es->ex_arglen) * sizeof(int)
				+ sizeof(int) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - pack.el_stringbase;
	} else {
		len = ((pack.el_argc + pack.el_envc + 2 + pack.el_es->ex_arglen) * sizeof(int)
				+ sizeof(char *) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - pack.el_stringbase;
	}

	len = ALIGN(len); /* make the stack "safely" aligned */

	if (len > pack.el_ssize) {
		error = ENOMEM;
		goto bad;
	}

	/* adjust "active stack depth" for process VSZ */
	pack.el_ssize = len;

	vm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
			VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);

	/* Map address Space  & create new process's VM space */
	error = exec_create_vmspace(p, &pack, base_vcp);
	if (error != 0) {
		goto exec_abort;
	}

	stack_base = exec_copyout_strings(&pack, &arginfo, vm, len, szsigcode, &error);
	if (error) {
		goto exec_abort;
	}

	stopprofclock(p);   	/* stop profiling */
	fdcloseexec(); 	    	/* handle close on exec */
	execsigs(p); 	    	/* reset catched signals */
	p->p_ctxlink = NULL; 	/* reset ucontext link */

	/* set command name & other accounting info */
	len = min(ndp.ni_cnd.cn_namelen, MAXCOMLEN);
	memcpy(p->p_comm, ndp.ni_cnd.cn_nameptr, len);
	p->p_comm[len] = 0;
	p->p_acflag &= ~AFORK;

	/* record proc's vnode, for use by procfs and others */
	if (p->p_textvp) {
		vrele(p->p_textvp);
	}
	VREF(pack.el_vnodep);
	p->p_textvp = pack.el_vnodep;

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

	kmem_free_wakeup(exec_map, (vm_offset_t)pack.el_stringbase, exec_maxhdrsz);

	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);
	vn_lock(pack.el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(pack.el_vnodep, FREAD, p->p_ucred, p);
	vput(pack.el_vnodep);

	/* setup new registers and do misc. setup. */
	(*pack.el_es->ex_emul->e_setregs)(p, &pack, (u_long)stack_base);

	if (p->p_flag & P_TRACED) {
		psignal(p, SIGTRAP);
	}

	/* update p_emul, the old value is no longer needed */
	p->p_emul = pack.el_es->ex_emul;
	/* ...and the same for p_execsw */
	p->p_execsw = pack.el_es;
	FREE(pack.el_image_hdr, M_EXEC);

#ifdef KTRACE
	if (KTRPOINT(p, KTR_EMUL)) {
		ktremul(p, p->p_emul->e_name);
	}
#endif

	p->p_flag &= ~P_INEXEC;
	return (EJUSTRETURN);

bad:
	p->p_flag &= ~P_INEXEC;
	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&pack.el_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (pack.el_flags & EXEC_HASFD) {
		pack.el_flags &= ~EXEC_HASFD;
		(void)fdrelease(pack.el_fd);
	}
	/* close and put the exec'd file */
	vn_lock(pack.el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(pack.el_vnodep, FREAD, p->p_ucred, p);
	vput(pack.el_vnodep);
	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);

freehdr:
	p->p_flag &= ~P_INEXEC;
	FREE(pack.el_image_hdr, M_EXEC);
	return (error);

exec_abort:
	p->p_flag &= ~P_INEXEC;
	vm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
			VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
	if (pack.el_emul_arg) {
		FREE(pack.el_emul_arg, M_TEMP);
	}
	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);
	vn_lock(pack.el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(pack.el_vnodep, FREAD, p->p_ucred, p);
	vput(pack.el_vnodep);
	if (pack.el_stringbase != NULL) {
		kmem_free_wakeup(exec_map, (vm_offset_t)pack.el_stringbase, exec_maxhdrsz);
	}
	FREE(pack.el_image_hdr, M_EXEC);
	exit(W_EXITCODE(0, SIGABRT));
	exit(-1);
	return (0);
}

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
check_exec(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	int	error, i;
	struct vnode *vp;
	struct nameidata *ndp;
	size_t resid;

	ndp = &elp->el_ndp;
	ndp->ni_cnd.cn_nameiop = LOOKUP;
	ndp->ni_cnd.cn_flags = FOLLOW | LOCKLEAF | SAVENAME;
	/* first get the vnode */
	if ((error = namei(ndp)) != 0) {
		return error;
	}
	elp->el_vnodep = vp = ndp->ni_vp;

	/* check access and type */
	if (vp->v_type != VREG) {
		error = EACCES;
		goto bad1;
	}
	if ((error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p)) != 0) {
		goto bad1;
	}

	/* get attributes */
	if ((error = VOP_GETATTR(vp, elp->el_attr, p->p_ucred, p)) != 0) {
		goto bad1;
	}

	/* Check mount point */
	if (vp->v_mount->mnt_flag & MNT_NOEXEC) {
		error = EACCES;
		goto bad1;
	}
	if (vp->v_mount->mnt_flag & MNT_NOSUID) {
		elp->el_attr->va_mode &= ~(S_ISUID | S_ISGID);
	}

	/* try to open it */
	if ((error = VOP_OPEN(vp, FREAD, p->p_ucred, p)) != 0) {
		goto bad1;
	}

	/* unlock vp, since we need it unlocked from here on out. */
	VOP_UNLOCK(vp, 0, p);

	error = vn_rdwr(UIO_READ, vp, elp->el_image_hdr, elp->el_hdrlen, 0,
				UIO_SYSSPACE, 0, p->p_ucred, &resid, NULL);

	if (error) {
		goto bad2;
	}
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
		newerror = (*execsw[i].ex_makecmds)(p, elp);
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

/*
 * Creates new vmspace from running vmcmd
 * Do whatever is necessary to prepare the address space
 * for remapping.  Note that this might replace the current
 * vmspace with another!
 */
static int
exec_create_vmspace(p, elp, base_vcp)
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
			if (base_vcp == NULL) {
				panic("execve: relative vmcmd with no base");
			}
			if (vcp->ev_flags & VMCMD_BASE) {
				panic("execve: illegal base & relative vmcmd");
			}
#endif
			vcp->ev_addr += base_vcp->ev_addr;
		}
		error = (*vcp->ev_proc)(p, vcp);
#ifdef DEBUG
		if (error) {
			if (i > 0) {
				printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i - 1, vcp[-1].ev_addr,
						vcp[-1].ev_size, vcp[-1].ev_offset);
			}
			printf("vmcmd[%d] = %#lx/%#lx @ %#lx\n", i, vcp->ev_addr,
					vcp->ev_size, vcp->ev_offset);
		}
#endif
		if (vcp->ev_flags & VMCMD_BASE) {
			base_vcp = vcp;
		}
	}

	/* free the vmspace-creation commands, and release their references */
	kill_vmcmds(&elp->el_vmcmds);
	if (error) {
		return (error);
	} else {
		return (0);
	}
}

/*
 * Copy out argument and environment strings from the old process
 *	address space into the temporary string buffer.
 */
static char *
exec_extract_strings(elp, argp, envp, length, retval)
	struct exec_linker *elp;
	char **argp, **envp;
	int length, *retval;
{
	char **argv, **envv, **tmpfap;
	char *dp, *sp;
	int error;

	dp = elp->el_stringbase;

	/* copy the fake args list, if there's one, freeing it as we go */
	if (elp->el_flags & EXEC_HASARGL) {
		tmpfap = elp->el_fa;
		while (*tmpfap != NULL) {
			char *cp;

			cp = *tmpfap;
			while (*cp) {
				*dp++ = *cp++;
			}
			dp++;
			FREE(*tmpfap, M_EXEC);
			tmpfap++;
			elp->el_argc++;
		}
		FREE(elp->el_fa, M_EXEC);
		elp->el_flags &= ~EXEC_HASARGL;
	}

	/* extract arguments first */
	argv = argp;
	if (argv) {
		if (elp->el_flags & EXEC_SKIPARG) {
			argv++;
		}
		while (1) {
			length = elp->el_stringbase + ARG_MAX - dp;
			if ((error = copyin(argv, &sp, sizeof(sp))) != 0) {
				error = EFAULT;
				goto bad;
			}
			if (!sp) {
				break;
			}
			if ((error = copyinstr(sp, dp, length, &length)) != 0) {
				if (error == ENAMETOOLONG) {
					error = E2BIG;
				}
				goto bad;
			}
			dp += length;
			argv++;
			elp->el_argc++;
		}
	} else {
		error = EINVAL;
		goto bad;
	}

	/* extract environment strings */
	envv = envp;
	if (envv) {
		while (1) {
			length = elp->el_stringbase + ARG_MAX - dp;
			if ((error = copyin(envv, &sp, sizeof(sp))) != 0) {
				error = EFAULT;
				goto bad;
			}
			if (!sp) {
				break;
			}
			if ((error = copyinstr(sp, dp, length, &length)) != 0) {
				if (error == ENAMETOOLONG) {
					error = E2BIG;
				}
				goto bad;
			}
			dp += length;
			envv++;
			elp->el_envc++;
		}
	} else {
		error = EINVAL;
		goto bad;
	}

	retval = &error;
	dp = (char *)ALIGN(dp);
	return (dp);

bad:
	retval = &error;
	dp = (char *)NULL;
	return (dp);
}

/*
 * Copy strings out to the new process address space, constructing
 * new arg and env vector tables. Return a pointer to the base
 * so that it can be used as the initial stack pointer.
 */
static char *
exec_copyout_strings(elp, arginfo, vm, length, szsigcode, retval)
	struct exec_linker *elp;
	struct ps_strings *arginfo;
	struct vmspace *vm;
	int length, szsigcode, *retval;
{
	char *stack_base;
	int error;

	/* Calculate string base and vector table pointers. */
	stack_base = (caddr_t)(vm->vm_minsaddr - length);

	/* Now copy argc, args & environ to new stack */
	error = (*elp->el_es->ex_copyargs)(elp, arginfo, stack_base, elp->el_stringbase);
	if (!error) {
		goto out;
	}

	/* copy out the process's ps_strings structure */
	error = copyout(&arginfo, (char *)PS_STRINGS, sizeof(arginfo));
	if (error) {
		goto out;
	}

	/* copy out the process's signal trapoline code */
	error = copyout((char *)elp->el_es->ex_emul->e_sigcode, ((char *)PS_STRINGS) - szsigcode, szsigcode);
	if (szsigcode && error) {
		goto out;
	}

out:
	retval = &error;
	return (stack_base);
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
	argc = elp->el_argc;
	envc = elp->el_envc;
	arginfo->ps_nargvstr = argc;
	arginfo->ps_nenvstr = envc;

	dp = (char *)(cpp +  1 + argc + 1+ envc) + elp->el_es->ex_arglen;
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

	stack = (char*) cpp;
	return (0);
}
