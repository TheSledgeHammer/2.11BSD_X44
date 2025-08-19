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

int
execve_load(p, uap, elp)
	struct proc *p;
	struct execa_args *uap;
	struct exec_linker *elp;
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

	/* allocate an argument buffer */
	elp->el_stringspace = (char *)kmem_alloc_wait(exec_map, exec_maxhdrsz);
	if (elp->el_stringbase == NULL) {
		error = ENOMEM;
		goto exec_abort;
	}
	elp->el_stringp = elp->el_stringbase;
	elp->el_stringspace = exec_maxhdrsz;
	MALLOC(elp->el_image_hdr, void *, (elp->el_stringspace + exec_maxhdrsz), M_EXEC, M_WAITOK);

	/* see if we can run it. */
	if ((error = check_exec(&elp)) != 0) {
		goto freehdr;
	}

	error = exec_extract_strings(&elp, dp);
	if (error != 0) {
		goto bad;
	}

	szsigcode = elp->el_es->ex_emul->e_esigcode - elp->el_es->ex_emul->e_sigcode;

	if (elp->el_flags & EXEC_32) {
		len = ((elp->el_argc + elp->el_envc + 2 + elp->el_es->ex_arglen) * sizeof(int)
				+ sizeof(int) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - elp->el_stringspace;
	} else {
		len = ((elp->el_argc + elp->el_envc + 2 + elp->el_es->ex_arglen) * sizeof(int)
				+ sizeof(char *) + dp + STACKGAPLEN + szsigcode
				+ sizeof(struct ps_strings)) - elp->el_stringspace;
	}

	len = ALIGN(len); /* make the stack "safely" aligned */

	if (len > elp->el_ssize) {
		error = ENOMEM;
		goto bad;
	}

	/* adjust "active stack depth" for process VSZ */
	elp->el_ssize = len;

	/* Map address Space  & create new process's VM space */
	error = vmcmd_create_vmspace(p, elp, base_vcp);
	if (error != 0) {
		goto exec_abort;
	}

	stack_base = (char *)exec_copyout_strings(&elp, &arginfo);
	/* Now copy argc, args & environ to new stack */
	error = (*elp->el_es->ex_copyargs)(&elp, &arginfo, stack_base, argp);
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
	if (elp->el_stringbase != NULL) {
		kmem_free_wakeup(exec_map, elp->el_stringbase, exec_maxhdrsz);
	}
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
		kmem_free_wakeup(exec_map, elp->el_stringbase, exec_maxhdrsz);
	}
	FREE(elp->el_image_hdr, M_EXEC);
	exit(W_EXITCODE(0, SIGABRT));
	exit(-1);
	return (0);
}
