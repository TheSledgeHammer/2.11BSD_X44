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
#include <sys/malloc.h>

void
getpid()
{
	u->u_r.r_val1 = u->u_procp->p_pid;
	u->u_r.r_val2 = u->u_procp->p_ppid;	/* XXX - compatibility */

#if defined(COMPAT_43)
	u->u_r.r_val2 = u->u_procp->p_pptr->p_pid;
#endif
}

void
getppid()
{
	u->u_r.r_val1 = u->u_procp->p_ppid;
}

void
getpgrp()
{
	register struct a {
		syscallarg(int)	pid;
	} *uap = (struct a *)u->u_ap;
	register struct proc *p;

	if (uap->pid == 0) {
		uap->pid = u->u_procp->p_pid;
	}
	p = pfind(uap->pid);
	if (p == 0) {
		u->u_error = ESRCH;
		goto retry;
	} else {
		goto out;
	}

retry:
	uap->pid = u->u_procp->p_pgrp->pg_id;
	p = pfind(uap->pid);
	if(p == 0) {
		u->u_error = ESRCH;
		return;
	} else {
		goto out;
	}

out:
	u->u_r.r_val1 = p->p_pgrp;
}

void
getuid()
{
	u->u_r.r_val1 = u->u_pcred->p_ruid;
	u->u_r.r_val2 = u->u_ucred->cr_uid;		/* XXX */
}

void
geteuid()
{

	u->u_r.r_val1 = u->u_ucred->cr_uid;
}

void
getgid()
{
	u->u_r.r_val1 = u->u_pcred->p_ruid;
	u->u_r.r_val2 = u->u_ucred->cr_groups[0];		/* XXX */
}

void
getegid()
{
	u->u_r.r_val1 = u->u_ucred->cr_groups[0];
}

/*
 * getgroups and setgroups differ from 4.X because the VAX stores group
 * entries in the user structure as shorts and has to convert them to ints.
 */
void
getgroups()
{
	register struct a {
		syscallarg(u_int) gidsetsize;
		syscallarg(int *) gidset;
	} *uap = (struct a*) u->u_ap;
	register gid_t *gp;

	for(gp = u->u_ucred->cr_groups[NGROUPS]; gp > u->u_ucred->cr_groups; gp--) {
		if (gp[-1] != NOGROUP) {
			break;
		}
	}
	if (uap->gidsetsize < gp - u->u_ucred->cr_groups) {
		u->u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u->u_ucred->cr_groups;
	u->u_error = copyout((caddr_t)u->u_ucred->cr_groups, (caddr_t)uap->gidset, uap->gidsetsize * sizeof(u->u_ucred->cr_groups[0]));
	if (u->u_error) {
		return;
	}
	u->u_r.r_val1 = uap->gidsetsize;
}

void
setpgrp()
{
	register struct proc *p;
	register struct a {
		syscallarg(int)	pid;
		syscallarg(int)	pgrp;
	} *uap = (struct a *)u->u_ap;

	if (uap->pid == 0)		/* silly... */
		uap->pid = u->u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u->u_error = ESRCH;
		return;
	}
/* need better control mechanisms for process groups */
	if (p->p_uid != u->u_ucred->cr_uid && u->u_ucred->cr_uid && !inferior(p)) {
		u->u_error = EPERM;
		return;
	}
	p->p_pgrp = uap->pgrp;
}

int
setpgid()
{
	register struct a {
		syscallarg(pid_t) pid;
		syscallarg(pid_t) pgid;
	}*uap = (struct a *) u->u_ap;

	register struct proc *targp;		/* target process */
	register struct pgrp *pgrp;			/* target pgrp */

	if(uap->pid != 0 && uap->pid != u->u_procp->p_pid) {
		if((targp = pfind(uap->pid)) == 0 || !inferior(targp)) {
			return (u->u_error = ESRCH);
		}
		if(targp->p_session != u->u_procp->p_session) {
			return (u->u_error = EPERM);
		}
		if (targp->p_flag & P_EXEC) {

			return (u->u_error = EACCES);
		}
	} else {
		targp = u->u_procp;
	}
	if(SESS_LEADER(targp)) {
		return (u->u_error = EPERM);
	}
	if(uap->pgid == 0) {
		uap->pgid = targp->p_pid;
	} else if(uap->pgid != targp->p_pid) {
		if((pgrp = pgfind(uap->pgid)) == 0 || pgrp->pg_session != u->u_procp->p_session) {
			return (u->u_error = EPERM);
		}
	}
	return (u->u_error = enterpgrp(targp, uap->pgid, 0));
}

void
setreuid()
{
	struct a {
		syscallarg(int)	ruid;
		syscallarg(int)	euid;
	} *uap = (struct a *)u.u_ap;

	register int ruid, euid;
	ruid = uap->ruid;

	if (ruid == -1) {
		ruid = u->u_pcred->p_ruid;
	}
	if (u->u_pcred->p_ruid != ruid && u->u_ucred->cr_uid != ruid && !suser()) {
		return;
	}
	euid = uap->euid;
	if (euid == -1) {
		euid = u->u_ucred->cr_uid;
	}
	if (u->u_pcred->p_ruid != euid && u->u_ucred->cr_uid != euid && !suser()) {
		return;
	}

	u->u_procp->p_uid = ruid;
	u->u_pcred->p_ruid = ruid;
	u->u_ucred->cr_uid = euid;
}

void
setregid()
{
	register struct a {
		syscallarg(int) rgid;
		syscallarg(int) egid;
	} *uap = (struct a*) u->u_ap;
	register int rgid, egid;

	rgid = uap->rgid;
	if (rgid == -1) {
		rgid = u->u_pcred->p_rgid;
	}
	if (u->u_pcred->p_rgid != rgid && u->u_ucred->cr_gid != rgid && !suser()) {
		return;
	}
	egid = uap->egid;
	if (egid == -1) {
		egid = u->u_ucred->cr_gid;
	}
	if (u->u_pcred->p_rgid != egid && u->u_ucred->cr_gid != egid && !suser()) {
		return;
	}
	if (u->u_pcred->p_rgid != rgid) {
		leavegroup(u->u_pcred->p_rgid);
		(void) entergroup(rgid);
		u->u_pcred->p_rgid = rgid;
	}
	u->u_ucred->cr_gid = egid;
}

void
setgroups()
{
	register struct	a {
		syscallarg(u_int) gidsetsize;
		syscallarg(int)	  *gidset;
	} *uap = (struct a *)u->u_ap;

	register gid_t *gp;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u->u_ucred->cr_groups) / sizeof (u->u_ucred->cr_groups[0])) {
		u->u_error = EINVAL;
		return;
	}
	u->u_error = copyin((caddr_t)uap->gidset, (caddr_t)u->u_ucred->cr_groups, uap->gidsetsize * sizeof (u->u_ucred->cr_groups[0]));
	if (u->u_error)
		return;
	for (gp = &u->u_ucred->cr_groups[uap->gidsetsize]; gp < &u->u_ucred->cr_groups[NGROUPS]; gp++) {
		*gp = NOGROUP;
	}
}

/*
 * Delete gid from the group set.
 */
void
leavegroup(gid)
	gid_t gid;
{
	register gid_t *gp;

	for (gp = u->u_ucred->cr_groups; gp < &u->u_ucred->cr_groups[NGROUPS]; gp++) {
		if (*gp == gid) {
			goto found;
		}
	}
	return;

found:
	for (; gp < &u->u_ucred->cr_groups[NGROUPS - 1]; gp++) {
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

	for (gp = u->u_ucred->cr_groups; gp < &u->u_ucred->cr_groups[NGROUPS]; gp++) {
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
	register struct a {
		syscallarg(char *) namebuf;
		syscallarg(u_int) namelen;
	} *uap = (struct a *)u->u_ap;

	register int error;
	if	(uap->namelen > sizeof (u->u_login))
		uap->namelen = sizeof (u->u_login);
	error = copyout(u->u_login, uap->namebuf, uap->namelen);
	return(u->u_error = error);
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
	register struct a {
		syscallarg(char *) namebuf;
	} *uap = (struct a *)u->u_ap;

	register int error;
	char	newname[MAXLOGNAME + 1];

	if	(!suser())
		return(u->u_error);	/* XXX - suser should be changed! */
/*
 * copinstr() wants to copy a string including a nul but u_login is not
 * necessarily nul-terminated.  Copy into a temp that is one character
 * longer then copy into place if that fit.
*/

	bzero(newname, sizeof (newname));
	error = copyinstr(uap->namebuf, newname, sizeof(newname), NULL);
	if	(error == 0)
		bcopy(newname, u->u_login, sizeof (u->u_login));
	return(u->u_error = error);
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
