/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_ktrace.c	8.5 (Berkeley) 5/14/95
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/ktrace.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/sysdecl.h>

#include <vm/include/vm_extern.h>

#ifdef KTRACE
int		ktrace_common(struct proc *, int, int, int, struct file *);
void	ktrinitheader(struct ktr_header *, struct proc *, int);
int		ktrops(struct proc *, struct proc *, int, int, struct file *);
int		ktrsetchildren(struct proc *, struct proc *, int, int, struct file *);
int 	ktrwrite(struct proc *, struct ktr_header *);
int		ktrcanset(struct proc *, struct proc *);
int		ktrsamefile(struct file *, struct file *);

int
ktrsamefile(f1, f2)
	struct file *f1, *f2;
{
	return ((f1 == f2) ||
	    ((f1 != NULL) && (f2 != NULL) &&
		(f1->f_type == f2->f_type) &&
		(f1->f_data == f2->f_data)));
}

void
ktrderef(p)
	struct proc *p;
{
	struct file *fp = p->p_tracep;
	p->p_traceflag = 0;
	if (fp == NULL)
		return;
	FILE_USE(fp);
	closef(fp);

	p->p_tracep = NULL;
}

void
ktradref(p)
	struct proc *p;
{
	struct file *fp = p->p_tracep;

	fp->f_count++;
}

void
ktrinitheader(kth, p, type)
	struct ktr_header *kth;
	struct proc *p;
	int type;
{
	bzero(kth, sizeof(*kth));
	kth->ktr_type = type;
	microtime(&kth->ktr_time);
	kth->ktr_pid = p->p_pid;
	bcopy(p->p_comm, kth->ktr_comm, MAXCOMLEN);
}

void
ktrsyscall(p, code, argsize, args)
	struct proc *p;
	int code, argsize;
	register_t args[];
{
	struct	ktr_header kth;
	struct	ktr_syscall *ktp;
	register long len = sizeof(struct ktr_syscall) + argsize;
	register_t *argp;
	int i;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_SYSCALL);
	MALLOC(ktp, struct ktr_syscall *, len, M_TEMP, M_WAITOK);
	ktp->ktr_code = code;
	ktp->ktr_argsize = argsize;
	argp = (register_t *)((char *)ktp + sizeof(struct ktr_syscall));
	for (i = 0; i < (argsize / sizeof *argp); i++)
		*argp++ = args[i];
	kth.ktr_buf = (caddr_t)ktp;
	kth.ktr_len = len;
	(void)ktrwrite(p, &kth);
	FREE(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrsysret(p, code, error, retval)
	struct proc *p;
	int code, error, retval;
{
	struct ktr_header kth;
	struct ktr_sysret ktp;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_SYSRET);
	ktp.ktr_code = code;
	ktp.ktr_error = error;
	ktp.ktr_retval = retval;		/* what about val2 ? */

	kth.ktr_buf = (caddr_t)&ktp;
	kth.ktr_len = sizeof(struct ktr_sysret);

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrnamei(p, path)
	struct proc *p;
	char *path;
{
	struct ktr_header kth;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_NAMEI);
	kth.ktr_len = strlen(path);
	kth.ktr_buf = path;

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktremul(p, emul)
	struct proc *p;
	char *emul;
{
	struct ktr_header kth;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_EMUL);
	kth.ktr_len = strlen(emul);
	kth.ktr_buf = emul;

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrgenio(p, fd, rw, iov, len, error)
	struct proc *p;
	int fd;
	enum uio_rw rw;
	register struct iovec *iov;
	int len, error;
{
	struct ktr_header kth;
	register struct ktr_genio *ktp;
	register caddr_t cp;
	register int resid = len, cnt;
	int buflen;

	if (error)
		return;
	p->p_traceflag |= KTRFAC_ACTIVE;
	buflen = (len + sizeof(struct ktr_genio));
	ktrinitheader(&kth, p, KTR_GENIO);
	MALLOC(ktp, struct ktr_genio *, buflen,	M_TEMP, M_WAITOK);
	ktp->ktr_fd = fd;
	ktp->ktr_rw = rw;

	kth.ktr_buf = (caddr_t)ktp;

	cp = (caddr_t)((char *)ktp + sizeof (struct ktr_genio));
	buflen -= sizeof(struct ktr_genio);

	while (resid > 0) {
		cnt = min(iov->iov_len, buflen);
		if (cnt > resid)
			cnt = resid;
		if (copyin(iov->iov_base, cp, (unsigned)cnt))
			break;
		kth.ktr_len = cnt + sizeof(struct ktr_genio);
		if (__predict_false(ktrwrite(p, &kth) != 0))
			break;

		iov->iov_base = (caddr_t)iov->iov_base + cnt;
		iov->iov_len -= cnt;

		if (iov->iov_len == 0)
			iov++;

		resid -= cnt;
	}

	FREE(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrpsig(p, sig, action, mask, code)
	struct proc *p;
	int sig;
	sig_t action;
	int mask, code;
{
	struct ktr_header kth;
	struct ktr_psig	kp;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_PSIG);
	kp.signo = (char)sig;
	kp.action = action;
	kp.mask = mask;
	kp.code = code;
	kth.ktr_buf = (caddr_t)&kp;
	kth.ktr_len = sizeof (struct ktr_psig);

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktrcsw(p, out, user)
	struct proc *p;
	int out, user;
{
	struct ktr_header kth;
	struct ktr_csw kc;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_PSIG);
	kc.out = out;
	kc.user = user;
	kth.ktr_buf = (caddr_t)&kc;
	kth.ktr_len = sizeof(struct ktr_csw);

	(void) ktrwrite(p, &kth);
	p->p_traceflag &= ~KTRFAC_ACTIVE;
}

void
ktruser(p, id, addr, len, ustr)
	struct proc *p;
	const char *id;
	void *addr;
	size_t len;
	int ustr;
{
	struct ktr_header kth;
	struct ktr_user *ktp;
	caddr_t user_dta;

	p->p_traceflag |= KTRFAC_ACTIVE;
	ktrinitheader(&kth, p, KTR_USER);
	MALLOC(ktp, struct ktr_user *, sizeof(struct ktr_user) + len, M_TEMP, M_WAITOK);
	if (ustr) {
		if (copyinstr(id, ktp->ktr_id, KTR_USER_MAXIDLEN, NULL) != 0)
			ktp->ktr_id[0] = '\0';
	} else
		strncpy(ktp->ktr_id, id, KTR_USER_MAXIDLEN);
	ktp->ktr_id[KTR_USER_MAXIDLEN - 1] = '\0';

	user_dta = (caddr_t)((char*) ktp + sizeof(struct ktr_user));
	if (copyin(addr, (void*) user_dta, len) != 0)
		len = 0;

	kth.ktr_buf = (void*) ktp;
	kth.ktr_len = sizeof(struct ktr_user) + len;
	(void) ktrwrite(p, &kth);

	FREE(ktp, M_TEMP);
	p->p_traceflag &= ~KTRFAC_ACTIVE;

}

/* Interface and common routines */
int
ktrace_common(curp, ops, facs, pid, fp)
	struct proc *curp;
	int ops, facs, pid;
	struct file *fp;
{
	int ret = 0;
	int error = 0;
	int one = 1;
	int descend;
	struct proc *p;
	struct pgrp *pg;

	curp->p_traceflag |= KTRFAC_ACTIVE;
	descend = ops & KTRFLAG_DESCEND;
	facs = facs & ~((unsigned) KTRFAC_ROOT);

	/*
	 * Clear all uses of the tracefile
	 */
	if (KTROP(ops) == KTROP_CLEARFILE) {
		for (p = LIST_FIRST(&allproc); p != NULL;
		     p = LIST_NEXT(p, p_list)) {
			if (ktrsamefile(p->p_tracep, fp)) {
				if (ktrcanset(curp, p))
					ktrderef(p);
				else
					error = EPERM;
			}
		}
		goto done;
	}

	/*
	 * Mark fp non-blocking, to avoid problems from possible deadlocks.
	 */
    /*
	if (fp != NULL) {
		fp->f_flag |= FNONBLOCK;
		(*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&one, curp);
	}
	*/

	/*
	 * need something to (un)trace (XXX - why is this here?)
	 */
	if (!facs) {
		error = EINVAL;
		goto done;
	}
	/*
	 * do it
	 */
	if (pid < 0) {
		/*
		 * by process group
		 */
		pg = pgfind(-pid);
		if (pg == NULL) {
			error = ESRCH;
			goto done;
		}
		for (p = LIST_FIRST(&pg->pg_mem); p != NULL;
		     p = LIST_NEXT(p, p_pglist)) {
			if (descend)
				ret |= ktrsetchildren(curp, p, ops, facs, fp);
			else
				ret |= ktrops(curp, p, ops, facs, fp);
		}

	} else {
		/*
		 * by pid
		 */
		p = pfind(pid);
		if (p == NULL) {
			error = ESRCH;
			goto done;
		}
		if (descend)
			ret |= ktrsetchildren(curp, p, ops, facs, fp);
		else
			ret |= ktrops(curp, p, ops, facs, fp);
	}
	if (!ret)
		error = EPERM;
done:
	curp->p_traceflag &= ~KTRFAC_ACTIVE;
	return (error);
}

/*
 * ktrace system call
 */
/* ARGSUSED */
int
ktrace()
{
	register struct ktrace_args {
		syscallarg(char *) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	 } *uap = (struct ktrace_args *)u.u_ap;

	register struct vnode *vp = NULL;
	register struct proc *curp, *p;
	register struct file *fp = NULL;
	int fd;
	int ops = KTROP(SCARG(uap, ops));
	int error = 0;
	struct nameidata nd;

	curp = u.u_procp;
	curp->p_traceflag |= KTRFAC_ACTIVE;
	if (ops != KTROP_CLEAR) {
		/*
		 * an operation which requires a file argument.
		 */
		NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, SCARG(uap, fname),
		    curp);
		if ((error = vn_open(&nd, FREAD|FWRITE, 0))) {
			curp->p_traceflag &= ~KTRFAC_ACTIVE;
			return (error);
		}
		vp = nd.ni_vp;
		VOP_UNLOCK(vp, 0, p);
		if (vp->v_type != VREG) {
			(void) vn_close(vp, FREAD|FWRITE, curp->p_ucred, curp);
			curp->p_traceflag &= ~KTRFAC_ACTIVE;
			return (EACCES);
		}
		if((fp = falloc()) == NULL) {
			goto done;
		}
		fp->f_flag = FWRITE|FAPPEND;
		fp->f_type = DTYPE_VNODE;
//		fp->f_ops = &vnops;
		fp->f_data = (caddr_t)vp;
		vp = NULL;
	}
	error = ktrace_common(curp, SCARG(uap, ops), SCARG(uap, facs), SCARG(uap, pid), fp);

done:
	if (vp != NULL)
		(void) vn_close(vp, FWRITE, curp->p_ucred, curp);
	if (fp != NULL) {
		FILE_UNUSE(fp); 		/* release file */
		fdrelease(fd); 			/* release fd table slot */
	}
	return (error);
}

int
ktrops(curp, p, ops, facs, fp)
	struct proc *p, *curp;
	int ops, facs;
	struct file *fp;
{

	if (!ktrcanset(curp, p))
		return (0);
	if (ops == KTROP_SET) {
		if (p->p_tracep != fp) {
			/*
			 * if trace file already in use, relinquish
			 */
			ktrderef(p);
			p->p_tracep = fp;
			ktradref(p);
		}
		p->p_traceflag |= facs;
		if (curp->p_ucred->cr_uid == 0)
			p->p_traceflag |= KTRFAC_ROOT;
	} else {
		/* KTROP_CLEAR */
		if (((p->p_traceflag &= ~facs) & KTRFAC_MASK) == 0) {
			/* no more tracing */
			ktrderef(p);
		}
	}

	return (1);
}

int
ktrsetchildren(curp, top, ops, facs, fp)
	struct proc *curp, *top;
	int ops, facs;
	struct file  *fp;
{
	register struct proc *p;
	register int ret = 0;

	p = top;
	for (;;) {
		ret |= ktrops(curp, p, ops, facs, fp);
		/*
		 * If this process has children, descend to them next,
		 * otherwise do any siblings, and if done with this level,
		 * follow back up the tree (but not past top).
		 */
		if (LIST_FIRST(&p->p_children))
			p = LIST_FIRST(&p->p_children);
		else for (;;) {
			if (p == top) {
				return (ret);
			}
			if(LIST_NEXT(p, p_sibling)) {
				p = LIST_NEXT(p, p_sibling);
				break;
			}
			p = p->p_pptr;
		}
	}
	/*NOTREACHED*/
}

int
ktrwrite(p, kth)
	struct proc *p;
	register struct ktr_header *kth;
{
	struct uio auio;
	struct iovec aiov[2];
	struct file *fp = p->p_tracep;
	int error, tries;

	if (fp == NULL)
		return (0);
	auio.uio_iov = &aiov[0];
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_rw = UIO_WRITE;
	aiov[0].iov_base = (caddr_t)kth;
	aiov[0].iov_len = sizeof(struct ktr_header);
	auio.uio_resid = sizeof(struct ktr_header);
	auio.uio_iovcnt = 1;
	auio.uio_procp = (struct proc *)0;
	if (kth->ktr_len > 0) {
		auio.uio_iovcnt++;
		aiov[1].iov_base = kth->ktr_buf;
		aiov[1].iov_len = kth->ktr_len;
		auio.uio_resid += kth->ktr_len;
	}

	FILE_USE(fp);
	error = (*fp->f_ops->fo_write)(fp, &auio, fp->f_cred);
	FILE_UNUSE(fp);

	if (error == 0) {
		return (0);
	}
		
	/*
	 * If error encountered, give up tracing on this vnode.
	 */
	log(LOG_NOTICE, "ktrace write failed, errno %d, tracing stopped\n",
	    error);
	for (p = LIST_FIRST(&allproc); p != 0; p = LIST_NEXT(p, p_list)) {
		if (ktrsamefile(p->p_tracep, fp)) {
			ktrderef(p);
		}
	}
	return (error);
}

/*
 * Return true if caller has permission to set the ktracing state
 * of target.  Essentially, the target can't possess any
 * more permissions than the caller.  KTRFAC_ROOT signifies that
 * root previously set the tracing status on the target process, and
 * so, only root may further change it.
 *
 * TODO: check groups.  use caller effective gid.
 */
int
ktrcanset(callp, targetp)
	struct proc *callp, *targetp;
{
	register struct pcred *caller = callp->p_cred;
	register struct pcred *target = targetp->p_cred;

	if ((caller->pc_ucred->cr_uid == target->p_ruid &&
	     target->p_ruid == target->p_svuid &&
	     caller->p_rgid == target->p_rgid &&	/* XXX */
	     target->p_rgid == target->p_svgid &&
	     (targetp->p_traceflag & KTRFAC_ROOT) == 0) ||
	     caller->pc_ucred->cr_uid == 0)
		return (1);

	return (0);
}

/*
 * Put user defined entry to ktrace records.
 */
int
utrace()
{
	register struct utrace_args {
		syscallarg(const char *) label;
		syscallarg(void *) addr;
		syscallarg(size_t) len;
	} *uap = (struct utrace_args *)u.u_ap;
	register struct proc *p;

	p = u.u_procp;
	ktruser(p, SCARG(uap, label), SCARG(uap, addr), SCARG(uap, len), 1);
	return (0);
}
#endif
