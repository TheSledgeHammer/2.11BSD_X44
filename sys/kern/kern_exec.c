/*
 * Copyright (c) 1993, David Greenman
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
 *	This product includes software developed by David Greenman
 * 4. The name of the developer may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: kern_exec.c,v 1.9 1994/09/25 19:33:36 phk Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/exec.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/map.h>
#include <sys/syslog.h>
#include <vm/vm.h>
#include <vm/include/vm_kern.h>

#include <machine/reg.h>
#include "../sys/exec_linker.h"

static int exec_check_permissions(struct exec_linker *);

/*
 * execsw_set is constructed for us by the linker.  Each of the items
 * is a pointer to a `const struct execsw', hence the double pointer here.
 */
extern const struct linker_set execsw_set;
const struct execsw **execsw = (const struct execsw **)&execsw_set.ls_items[0];

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
		int *stack_base;
		int error, len, i;
		struct exec_linker elp_image, *elp;
		struct vnode *vnodep;
		struct vattr attr;
		char *image_header;
		elp = &elp_image;
		bzero((caddr_t)elp, sizeof(struct exec_linker));
		image_header = (char *)0;

		/* Initialize a few constants in the common area */
		elp->el_proc = p;
		elp->el_uap = uap;
		elp->el_attr = &attr;

		/* Allocate temporary demand zeroed space for argument and environment strings */
		error = vm_allocate(kernel_map, (vm_offset_t *)&elp->el_stringbase, ARG_MAX, TRUE);
		if(error) {
			log(LOG_WARNING, "execve: failed to allocate string space\n");
			return (u->u_error);
		}

		if (!elp->el_stringbase) {
			error = ENOMEM;
				goto exec_fail;
		}
		elp->el_stringp = elp->el_stringbase;
		elp->el_stringspace = ARG_MAX;

		/* Translate the file name. namei() returns a vnode pointer in ni_vp amoung other things. */
		ndp = &nd;
		ndp->ni_cnd.cn_nameiop = LOOKUP;
		ndp->ni_cnd.cn_flags = LOCKLEAF | FOLLOW | SAVENAME;
		ndp->ni_cnd.cn_proc = curproc;
		ndp->ni_cnd.cn_cred = curproc->p_cred->pc_ucred;
		ndp->ni_segflg = UIO_USERSPACE;
		ndp->ni_dirp = uap->fname;

interpret:

		error = namei(ndp);
		if (error) {
			vm_deallocate(kernel_map, (vm_offset_t)elp->el_stringbase, ARG_MAX);
			goto exec_fail;
		}

		elp->el_vnodep = vnodep = ndp->ni_vp;


		if (vnodep == NULL) {
			error = ENOEXEC;
			goto exec_fail_dealloc;
		}

		/* Check file permissions (also 'opens' file) */
		error = exec_check_permissions(elp);
		if (error)
			goto exec_fail_dealloc;

		/* Map the image header (first page) of the file into kernel address space */
		error = vm_mmap(kernel_map,				/* map */
				(vm_offset_t *)&image_header,	/* address */
				PAGE_SIZE,						/* size */
				VM_PROT_READ,					/* protection */
				VM_PROT_READ,					/* max protection */
				0,								/* flags */
				(caddr_t)vnodep,				/* vnode */
				0);								/* offset */
		if (error) {
			uprintf("mmap failed: %d\n", u->u_error);
			goto exec_fail_dealloc;
		}
		elp->el_image_hdr = image_header;


		/*
		 * Loop through list of image activators, calling each one.
		 *	If there is no match, the activator returns -1. If there
		 *	is a match, but there was an error during the activation,
		 *	the error is returned. Otherwise 0 means success. If the
		 *	image is interpreted, loop back up and try activating
		 *	the interpreter.
		 */
		for (i = 0; execsw[i]; ++i) {
			if (execsw[i]->ex_exec_linker)
				u->u_error = (*execsw[i]->ex_exec_linker)(elp);
			else
				continue;
			if (error == -1)
				continue;
			if (error)
				goto exec_fail_dealloc;
			if (elp->el_interpreted) {
				/* free old vnode and name buffer */
				vput(ndp->ni_vp);
				mfree(ndp->ni_cnd.cn_pnbuf);
				if (vm_deallocate(kernel_map, (vm_offset_t)image_header, PAGE_SIZE))
					panic("execve: header dealloc failed (1)");

				/* set new name to that of the interpreter */
				ndp->ni_segflg = UIO_SYSSPACE;
				ndp->ni_dirp = elp->el_interpreter_name;
				ndp->ni_cnd.cn_nameiop = LOOKUP;
				ndp->ni_cnd.cn_flags = LOCKLEAF | FOLLOW | SAVENAME;
				ndp->ni_cnd.cn_proc = curproc;
				ndp->ni_cnd.cn_cred = curproc->p_cred->pc_ucred;
				goto interpret;
			}
			break;
		}
		/* If we made it through all the activators and none matched, exit. */
		if (error == -1) {
			error = ENOEXEC;
			goto exec_fail_dealloc;
		}
		/* Copy out strings (args and env) and initialize stack base */
		stack_base = exec_copyout_strings(elp);
		p->p_vmspace->vm_minsaddr = (char *)stack_base;

		/* Stuff argument count as first item on stack */
		*(--stack_base) = elp->el_argc;

		/* close files on exec */
		fdcloseexec(p);

		/* reset caught signals */
		execsigs(p);

		/* name this process - nameiexec(p, ndp) */
		len = min(ndp->ni_cnd.cn_namelen, MAXCOMLEN);
		bcopy(ndp->ni_cnd.cn_nameptr, p->p_comm, len);
		p->p_comm[len] = 0;

		/*
		 * mark as executable, wakeup any process that was vforked and tell
		 * it that it now has it's own resources back
		 */
		p->p_flag |= P_EXEC;
		if (p->p_pptr && (p->p_flag & P_PPWAIT)) {
			p->p_flag &= ~P_PPWAIT;
			wakeup((caddr_t)p->p_pptr);
		}

		/* implement set userid/groupid */
		p->p_flag &= ~P_SUGID;

		/* Turn off kernel tracing for set-id programs, except for root. */
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

		/* Implement correct POSIX saved uid behavior. */
		p->p_cred->p_svuid = p->p_ucred->cr_uid;
		p->p_cred->p_svgid = p->p_ucred->cr_gid;

		/* mark vnode pure text */
		ndp->ni_vp->v_flag |= VTEXT;

		/* Store the vp for use in procfs */
		if (p->p_textvp)		/* release old reference */
			vrele(p->p_textvp);
		VREF(ndp->ni_vp);
		p->p_textvp = ndp->ni_vp;

		/*
		 * If tracing the process, trap to debugger so breakpoints
		 * 	can be set before the program executes.
		 */
		if (p->p_flag & P_TRACED)
			psignal(p, SIGTRAP);

		/* clear "fork but no exec" flag, as we _are_ execing */
		u->u_acflag &= ~AFORK;

		/* Set entry address */
		setregs(p, elp->el_entry, (u_long)stack_base);

		/* free various allocated resources */
		if (vm_deallocate(kernel_map, (vm_offset_t)elp->el_stringbase, ARG_MAX))
			panic("execve: string buffer dealloc failed (1)");
		if (vm_deallocate(kernel_map, (vm_offset_t)image_header, PAGE_SIZE))
			panic("execve: header dealloc failed (2)");
		vput(ndp->ni_vp);
		mfree(ndp->ni_cnd.cn_pnbuf);

		return (0);

exec_fail_dealloc:
		if (elp->el_stringbase && elp->el_stringbase != (char *)-1)
			if (vm_deallocate(kernel_map, (vm_offset_t)elp->el_stringbase, ARG_MAX))
				panic("execve: string buffer dealloc failed (2)");
		if (elp->el_image_hdr && elp->el_image_hdr != (char *)-1)
			if (vm_deallocate(kernel_map, (vm_offset_t)elp->el_image_hdr, PAGE_SIZE))
				panic("execve: header dealloc failed (3)");
		vput(ndp->ni_vp);
		rmfree(ndp->ni_cnd.cn_pnbuf);

exec_fail:
		if (elp->el_vmspace_destroyed) {
			/* sorry, no more process anymore. exit gracefully */
#if 0	/* XXX */
			vm_deallocate(&vs->vm_map, USRSTACK - MAXSSIZ, MAXSSIZ);
#endif
			exit1(p, W_EXITCODE(0, SIGABRT));
			/* NOT REACHED */
			return(0);
		} else {
			return (error);
		}
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

