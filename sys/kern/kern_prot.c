/*
 * Copyright (c) 1982, 1986, 1989, 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	@(#)kern_prot.c	8.9 (Berkeley) 2/14/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_prot.c	1.4 (2.11BSD GTE) 1997/11/28
 */

/*
 * System calls related to processes and protection
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/sysdecl.h>
#include <sys/malloc.h>

int	entergroup(gid_t);
void leavegroup(gid_t);

int
getpid()
{
	u.u_r.r_val1 = u.u_procp->p_pid;
	u.u_r.r_val2 = u.u_procp->p_ppid;	/* XXX - compatibility */

#if defined(COMPAT_43)
	u.u_r.r_val2 = u.u_procp->p_pptr->p_pid;
#endif
	return (0);
}

int
getppid()
{
	u.u_r.r_val1 = u.u_procp->p_ppid;
	return (0);
}

int
getpgrp()
{
	register struct getpgrp_args {
		syscallarg(int) pid;
	} *uap = (struct getpgrp_args*) u.u_ap;
	register struct proc *p;

	if (SCARG(uap, pid) == 0) {
		SCARG(uap, pid) = u.u_procp->p_pid;
	}
	p = pfind(SCARG(uap, pid));
	if (p == 0) {
		u.u_error = ESRCH;
		goto retry;
	} else {
		goto out;
	}

retry:
	SCARG(uap, pid) = u.u_procp->p_pgrp->pg_id;
	p = pfind(SCARG(uap, pid));
	if (p == 0) {
		u.u_error = ESRCH;
		return (u.u_error);
	} else {
		goto out;
	}

out:
	u.u_r.r_val1 = p->p_pgrp->pg_id;
	return (0);
}

int
getuid()
{ 
	u.u_r.r_val1 = u.u_pcred->p_ruid;
#if defined(COMPAT_43)
	u.u_r.r_val2 = u.u_ucred->cr_uid;		/* XXX */
#endif
	return (0);
}

int
geteuid()
{
	u.u_r.r_val1 = u.u_ucred->cr_uid;
	return (0);
}

int
getgid()
{
	u.u_r.r_val1 = u.u_pcred->p_ruid;
#if defined(COMPAT_43)
	u.u_r.r_val2 = u.u_ucred->cr_groups[0];		/* XXX */
#endif
	return (0);
}

int
getegid()
{
	u.u_r.r_val1 = u.u_ucred->cr_groups[0];
	return (0);
}

int
getpgid()
{
	register struct getpgid_args {
		syscallarg(int) pid;
	} *uap = (struct getpgid_args*) u.u_ap;
	register struct proc *p;

	if (SCARG(uap, pid) == 0) {
		SCARG(uap, pid) = p->p_pgid;
	}
	p = pfind(SCARG(uap, pid));
	if (p == 0) {
		u.u_error = ESRCH;
		return (u.u_error);
	}
	u.u_r.r_val1 = p->p_pgid;
	return (0);
}

int
getsid()
{
	register struct getsid_args {
		syscallarg(int) pid;
	} *uap = (struct getsid_args*) u.u_ap;
	register struct proc *p;

	if (SCARG(uap, pid) == 0) {
		SCARG(uap, pid) = p->p_session->s_leader->p_pid;
	}
	p = pfind(SCARG(uap, pid));
	if (p == 0) {
		u.u_error = ESRCH;
		return (u.u_error);
	}
	u.u_r.r_val1 = p->p_session->s_leader->p_pid;
	return (0);
}

/*
 * getgroups and setgroups differ from 4.X because the VAX stores group
 * entries in the user structure as shorts and has to convert them to ints.
 */
int
getgroups()
{
	register struct getgroups_args {
		syscallarg(u_int) gidsetsize;
		syscallarg(int *)   gidset;
	} *uap = (struct getgroups_args*) u.u_ap;
	register gid_t *gp;

	for(gp = &u.u_ucred->cr_groups[NGROUPS]; gp > u.u_ucred->cr_groups; gp--) {
		if (gp[-1] != NOGROUP) {
			break;
		}
	}
	if (SCARG(uap, gidsetsize) < gp - u.u_ucred->cr_groups) {
		u.u_error = EINVAL;
		return (u.u_error);
	}
	SCARG(uap, gidsetsize) = gp - u.u_ucred->cr_groups;
	u.u_error = copyout((caddr_t)u.u_ucred->cr_groups, (caddr_t)SCARG(uap, gidset), SCARG(uap, gidsetsize) * sizeof(u.u_ucred->cr_groups[0]));
	if (u.u_error) {
		return (u.u_error);
	}
	u.u_r.r_val1 = SCARG(uap, gidsetsize);
	return (0);
}

int
setpgrp()
{
	register struct setpgrp_args {
		syscallarg(int)	pid;
		syscallarg(int)	pgrp;
	} *uap = (struct setpgrp_args *)u.u_ap;
	register struct proc *p;

	if (SCARG(uap, pid) == 0)		/* silly... */
		SCARG(uap, pid) = u.u_procp->p_pid;
	p = pfind(SCARG(uap, pid));
	if (p == 0) {
		u.u_error = ESRCH;
		return (u.u_error);
	}
	/* need better control mechanisms for process groups */
	if (p->p_uid != u.u_ucred->cr_uid && u.u_ucred->cr_uid && !inferior(p)) {
		u.u_error = EPERM;
		return (u.u_error);
	}
	p->p_pgrp->pg_id = SCARG(uap, pgrp);
	return (0);
}

int
setpgid()
{
	register struct setpgid_args {
		syscallarg(pid_t) pid;
		syscallarg(pid_t) pgid;
	}*uap = (struct setpgid_args *) u.u_ap;

	register struct proc *targp;		/* target process */
	register struct pgrp *pgrp;			/* target pgrp */

	if(SCARG(uap, pid) != 0 && SCARG(uap, pid) != u.u_procp->p_pid) {
		if((targp = pfind(SCARG(uap, pid))) == 0 || !inferior(targp)) {
			return (u.u_error = ESRCH);
		}
		if(targp->p_session != u.u_procp->p_session) {
			return (u.u_error = EPERM);
		}
		if (targp->p_flag & P_EXEC) {

			return (u.u_error = EACCES);
		}
	} else {
		targp = u.u_procp;
	}
	if(SESS_LEADER(targp)) {
		return (u.u_error = EPERM);
	}
	if(SCARG(uap, pgid) == 0) {
		SCARG(uap, pgid) = targp->p_pid;
	} else if(SCARG(uap, pgid) != targp->p_pid) {
		if((pgrp = pgfind(SCARG(uap, pgid))) == 0 || pgrp->pg_session != u.u_procp->p_session) {
			return (u.u_error = EPERM);
		}
	}
	return (u.u_error = enterpgrp(targp, SCARG(uap, pgid), 0));
}

int
setreuid()
{
	struct setreuid_args {
		syscallarg(int)	ruid;
		syscallarg(int)	euid;
	} *uap = (struct setreuid_args *)u.u_ap;

	register int ruid, euid;
	ruid = SCARG(uap, ruid);

	if (ruid == -1) {
		ruid = u.u_pcred->p_ruid;
	}
	if (u.u_pcred->p_ruid != ruid && u.u_ucred->cr_uid != ruid && !suser()) {
		return (setuid());
	}
	euid = SCARG(uap, euid);
	if (euid == -1) {
		euid = u.u_ucred->cr_uid;
		return (0);
	}
	if (u.u_pcred->p_ruid != euid && u.u_ucred->cr_uid != euid && !suser()) {
		return (EPERM);
	}

	u.u_procp->p_uid = ruid;
	u.u_pcred->p_ruid = ruid;
	u.u_ucred->cr_uid = euid;

	return (seteuid());
}

int
setregid()
{
	register struct setregid_args {
		syscallarg(int) rgid;
		syscallarg(int) egid;
	} *uap = (struct setregid_args *) u.u_ap;
	register int rgid, egid;

	rgid = SCARG(uap, rgid);
	if (rgid == -1) {
		rgid = u.u_pcred->p_rgid;
	}
	if (u.u_pcred->p_rgid != rgid && u.u_ucred->cr_gid != rgid && !suser()) {
		return (setgid());
	}
	egid = SCARG(uap, egid);
	if (egid == -1) {
		egid = u.u_ucred->cr_gid;
		return (0);
	}
	if (u.u_pcred->p_rgid != egid && u.u_ucred->cr_gid != egid && !suser()) {
		return (EPERM);
	}
	if (u.u_pcred->p_rgid != rgid) {
		leavegroup(u.u_pcred->p_rgid);
		(void) entergroup(rgid);
		u.u_pcred->p_rgid = rgid;
	}
	u.u_ucred->cr_gid = egid;
	return (setegid());
}

int
setgroups()
{
	register struct	setgroups_args {
		syscallarg(u_int) gidsetsize;
		syscallarg(int *) gidset;
	} *uap = (struct setgroups_args *)u.u_ap;

	register gid_t *gp;
	int error;

	if (u.u_error == suser() || !suser())
		return (u.u_error);
	if (SCARG(uap, gidsetsize) > sizeof (u.u_ucred->cr_groups) / sizeof (u.u_ucred->cr_groups[0])) {
		u.u_error = EINVAL;
		return (u.u_error);
	}
	u.u_error = copyin((caddr_t)SCARG(uap, gidset), (caddr_t)u.u_ucred->cr_groups, SCARG(uap, gidsetsize) * sizeof (u.u_ucred->cr_groups[0]));
	if (u.u_error)
		return (u.u_error);
	for (gp = &u.u_ucred->cr_groups[SCARG(uap, gidsetsize)]; gp < &u.u_ucred->cr_groups[NGROUPS]; gp++) {
		*gp = NOGROUP;
	}
	return (0);
}

/*
 * Delete gid from the group set.
 */
void
leavegroup(gid)
	gid_t gid;
{
	register gid_t *gp;

	for (gp = u.u_ucred->cr_groups; gp < &u.u_ucred->cr_groups[NGROUPS]; gp++) {
		if (*gp == gid) {
			goto found;
		}
	}
	return;

found:
	for (; gp < &u.u_ucred->cr_groups[NGROUPS - 1]; gp++) {
		*gp = *(gp + 1);
	}
	*gp = NOGROUP;
}

/*
 * Add gid to the group set.
 */
int
entergroup(gid)
	gid_t gid;
{
	register gid_t *gp;

	for (gp = u.u_ucred->cr_groups; gp < &u.u_ucred->cr_groups[NGROUPS]; gp++) {
		if (*gp == gid) {
			return (0);
		}
		if (*gp == NOGROUP) {
			*gp = gid;
			return (0);
		}
	}
	return (-1);
}

/*
 * Get login name, if available.
*/
int
getlogin()
{
	register struct getlogin_args {
		syscallarg(char *) namebuf;
		syscallarg(u_int) namelen;
	} *uap = (struct getlogin_args *)u.u_ap;

	register int error;
	if	(SCARG(uap, namelen) > sizeof (u.u_login))
		SCARG(uap, namelen) = sizeof (u.u_login);
	error = copyout(u.u_login, SCARG(uap, namebuf), SCARG(uap, namelen));
	return (u.u_error = error);
}

/*
 * Set login name.
 * It is not clear whether this should be allowed if the process
 * is not the "session leader" (the 'login' process).  But since 2.11
 * doesn't have sessions and it's almost impossible to know if a process
 * is "login" or not we simply restrict this call to the super user.
*/

int
setlogin()
{
	register struct setlogin_args {
		syscallarg(char *) namebuf;
	} *uap = (struct setlogin_args *)u.u_ap;

	register int error;
	char	newname[MAXLOGNAME + 1];

	if (!suser())
		return (u.u_error); /* XXX - suser should be changed! */
	/*
	 * copinstr() wants to copy a string including a nul but u_login is not
	 * necessarily nul-terminated.  Copy into a temp that is one character
	 * longer then copy into place if that fit.
	 */

	bzero(newname, sizeof (newname));
	error = copyinstr(SCARG(uap, namebuf), newname, sizeof(newname), NULL);
	if (error == 0)
		bcopy(newname, u.u_login, sizeof (u.u_login));
	return (u.u_error = error);
}

/*
 * Test whether the specified credentials have the privilege
 * in question.
 */
int
priv_check(priv)
  int priv;
{
   	if (u.u_procp != NULL) {
   		return (priv_check_cred(u.u_ucred, priv, 0));
   	}
   	return (0);
}

/*
 * Check a credential for privilege.
 *
 * A non-null credential is expected unless NULL_CRED_OKAY is set.
 */
int
priv_check_cred(cred, priv, flags)
	struct ucred *cred;
	int priv, flags;
{
	int error;
	
	KASSERT(PRIV_VALID(priv));

	KASSERT(cred != NULL || (flags & NULL_CRED_OKAY));
		
	if (cred == NULL) {
		if (flags & NULL_CRED_OKAY)
			return (0);
		else
			return (EPERM);
	}
	if (cred->cr_uid != 0) 
		return (EPERM);
/*
	error = prison_priv_check(cred, priv);
	if (error)
		return (error);
*/
	/* NOTE: accounting for suser access (p_acflag/ASU) removed */
	return (0);
}

/*
 * Allocate a zeroed cred structure.
 */
struct ucred *
crget()
{
	register struct ucred *cr;

	MALLOC(cr, struct ucred *, sizeof(*cr), M_CRED, M_WAITOK);
	bzero((caddr_t)cr, sizeof(*cr));
	cr->cr_ref = 1;
	return (cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
void
crfree(cr)
	struct ucred *cr;
{
	int s;

	s = splimp();				/* ??? */
	if (--cr->cr_ref == 0)
		FREE((caddr_t)cr, M_CRED);
	(void) splx(s);
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	if (cr->cr_ref == 1)
		return (cr);
	newcr = crget();
	*newcr = *cr;
	crfree(cr);
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * Dup cred struct to a new held one.
 */
struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * Increment the reference count of a cred structure.
 * Returns the passed structure.
 */
struct ucred *
crhold(cr)
	struct ucred *cr;
{
	cr->cr_ref++;
	return (cr);
}
