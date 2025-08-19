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

int execve_load(struct proc *, struct execa_args *, struct exec_linker *, register_t *);

int
execve()
{
	struct proc *p;
	struct execa_args *uap;
	struct exec_linker elp;
	register_t *retval;

	p = u.u_procp;
	uap = (struct execa_args *)u.u_ap;
	return (execve_load(p, uap, &elp, retval));
}

int
execve_load(p, uap, elp, retval)
	struct proc *p;
	struct execa_args *uap;
	struct exec_linker *elp;
	register_t *retval;
{
	struct nameidata ndp;
	struct ps_strings arginfo;
	struct vmspace	*vm;
	struct vattr attr;
	struct exec_vmcmd *base_vcp;
	int error, szsigcode;
	size_t len;
	char *stack_base;
	char *dp;

	p->p_flag |= P_INEXEC;
    base_vcp = NULL;
    vm = p->p_vmspace;

	NDINIT(&ndp, LOOKUP, NOFOLLOW, UIO_USERSPACE, SCARG(uap, fname), p);

	/* Initialize a few constants in the common area */
	elp->el_name = SCARG(uap, fname);
	MALLOC(elp->el_image_hdr, void *, exec_maxhdrsz, M_EXEC, M_WAITOK);
	elp->el_hdrlen = exec_maxhdrsz;
	elp->el_hdrvalid = 0;
	elp->el_proc = p;
	elp->el_uap = uap;
	elp->el_attr = &attr;
	elp->el_argc = 0;
	elp->el_envc = 0;
	elp->el_entry = 0;
	elp->el_ndp = ndp;
	elp->el_emul_arg = NULL;
	elp->el_vmcmds.evs_cnt = 0;
	elp->el_vmcmds.evs_used = 0;
	elp->el_flags = 0;

	/* see if we can run it. */
	if ((error = check_exec(&elp)) != 0) {
		goto freehdr;
	}

	/* allocate an argument buffer */
	elp->el_stringbase = (char *)kmem_alloc_wait(exec_map, exec_maxhdrsz);
	if (elp->el_stringbase == NULL) {
		error = ENOMEM;
		goto exec_abort;
	}
	elp->el_stringp = elp->el_stringbase;
	elp->el_stringspace = exec_maxhdrsz;

	dp = exec_extract_strings(&elp, SCARG(uap, argp), SCARG(uap, envp), len, &error);
	if ((dp == NULL) && (error != 0)) {
		goto bad;
	}

	szsigcode = elp->el_es->ex_emul->e_esigcode - elp->el_es->ex_emul->e_sigcode;

	if (elp->el_flags & EXEC_32) {
		len = ((elp->el_argc + elp->el_envc + 2 + elp->el_es->ex_arglen) * sizeof(int)
				+ sizeof(int) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - elp->el_stringbase;
	} else {
		len = ((elp->el_argc + elp->el_envc + 2 + elp->el_es->ex_arglen) * sizeof(int)
				+ sizeof(char *) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - elp->el_stringbase;
	}

	len = ALIGN(len); /* make the stack "safely" aligned */

	if (len > elp->el_ssize) {
		error = ENOMEM;
		goto bad;
	}

	/* adjust "active stack depth" for process VSZ */
	elp->el_ssize = len;

	vm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
			VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);

	/* Map address Space  & create new process's VM space */
	error = vmcmd_create_vmspace(p, elp, base_vcp);
	if (error != 0) {
		goto exec_abort;
	}

	stack_base = (char *)exec_copyout_strings(&elp, &arginfo);
	/* Now copy argc, args & environ to new stack */
	error = (*elp->el_es->ex_copyargs)(&elp, &arginfo, stack_base, elp->el_stringbase);
	if (error) {
		goto exec_abort;
	}

	/* copy out the process's ps_strings structure */
	if (copyout(&arginfo, (char *)PS_STRINGS, sizeof(arginfo))) {
		goto exec_abort;
	}

	/* copy out the process's signal trapoline code */
	if (szsigcode
			&& copyout((char *)elp->el_es->ex_emul->e_sigcode,
					((char *)PS_STRINGS) - szsigcode, szsigcode)) {
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
	VREF(elp->el_vnodep);
	p->p_textvp = elp->el_vnodep;

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

	kmem_free_wakeup(exec_map, (vm_offset_t)elp->el_stringbase, exec_maxhdrsz);

	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);
	vn_lock(elp->el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(elp->el_vnodep, FREAD, p->p_ucred, p);
	vput(elp->el_vnodep);

	/* setup new registers and do misc. setup. */
	(*elp->el_es->ex_emul->e_setregs)(p, &elp, (u_long)stack_base);

	if (p->p_flag & P_TRACED) {
		psignal(p, SIGTRAP);
	}

	/* update p_emul, the old value is no longer needed */
	p->p_emul = elp->el_es->ex_emul;
	/* ...and the same for p_execsw */
	p->p_execsw = elp->el_es;
	FREE(elp->el_image_hdr, M_EXEC);

	return (EJUSTRETURN);

bad:
	p->p_flag &= ~P_INEXEC;
	/* free the vmspace-creation commands, and release their references */
	kill_vmcmd(&elp->el_vmcmds);
	/* kill any opened file descriptor, if necessary */
	if (elp->el_flags & EXEC_HASFD) {
		elp->el_flags &= ~EXEC_HASFD;
		(void)fdrelease(elp->el_fd);
	}
	/* close and put the exec'd file */
	vn_lock(elp->el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(elp->el_vnodep, FREAD, p->p_ucred, p);
	vput(elp->el_vnodep);
	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);

freehdr:
	p->p_flag &= ~P_INEXEC;
	FREE(elp->el_image_hdr, M_EXEC);
	return (error);

exec_abort:
	p->p_flag &= ~P_INEXEC;
	vm_deallocate(&vm->vm_map, VM_MIN_ADDRESS,
			VM_MAXUSER_ADDRESS - VM_MIN_ADDRESS);
	if (elp->el_emul_arg) {
		FREE(elp->el_emul_arg, M_TEMP);
	}
	FREE(ndp.ni_cnd.cn_pnbuf, M_NAMEI);
	vn_lock(elp->el_vnodep, LK_EXCLUSIVE | LK_RETRY, p);
	VOP_CLOSE(elp->el_vnodep, FREAD, p->p_ucred, p);
	vput(elp->el_vnodep);
	if (elp->el_stringbase != NULL) {
		kmem_free_wakeup(exec_map, (vm_offset_t)elp->el_stringbase, exec_maxhdrsz);
	}
	FREE(elp->el_image_hdr, M_EXEC);
	exit(W_EXITCODE(0, SIGABRT));
	exit(-1);
	return (0);
}

/*
 * Copy out argument and environment strings from the old process
 *	address space into the temporary string buffer.
 */
char *
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
int *
exec_copyout_strings(elp, arginfo)
	struct exec_linker *elp;
	struct ps_strings *arginfo;
{
	long argc, envc;
	char **vectp;
	char *stringp, *destp;
	int *stack_base;

	/* Calculate string base and vector table pointers. */
	arginfo = PS_STRINGS;
	destp = (caddr_t)arginfo - roundup((ARG_MAX - elp->el_stringspace), sizeof(char *));

	/* The '+ 2' is for the null pointers at the end of each of the arg and	env vector sets */
	vectp = (char **)(destp - (elp->el_argc + elp->el_envc + 2) * sizeof(char *));

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
		while ((*destp++ = *stringp++)) {
			;
		}
	}

	/* a null vector table pointer seperates the argp's from the envp's */
	*(vectp++) = NULL;

	arginfo->ps_envstr = destp;
	arginfo->ps_nenvstr = envc;

	/* Copy the env strings and fill in vector table as we go. */
	for (; envc > 0; --envc) {
		*(vectp++) = destp;
		while ((*destp++ = *stringp++)) {
			;
		}
	}

	/* end of vector table is a null pointer */
	*vectp = NULL;
	return (stack_base);
}
