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
 *	@(#)kern_prot2.c  8.9.2 (2.11BSD) 2000/2/20
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/ucred.h>
#include <sys/acct.h>
#include <sys/proc.h>
#include <sys/systm.h>

int
setuid()
{
	struct a {
		uid_t uid;
	} *uap = (struct a *)u->u_ap;

	return(_setuid(uap->uid));
}

/*
 * This is a helper function used by setuid() above and the 4.3BSD 
 * compatibility code.  When the latter goes away this can be joined
 * back into the above code and save a function call.
*/
int
_setuid(uid)
	register uid_t uid;
{
	if (uid != u->u_pcred->p_ruid && !suser())
		return(u->u_error);
	u->u_procp->p_uid = uid;
	u->u_ucred->cr_uid = uid;
	u->u_pcred->p_ruid = uid;
	u->u_pcred->p_svuid = uid;
	//u.u_acflag |= ASUGID;
	return (u->u_error = 0);
}

int
seteuid()
{
	struct a {
		uid_t euid;
	} *uap = (struct a *)u->u_ap;

	return(_seteuid(uap->euid));
}

int
_seteuid(euid)
	register uid_t euid;
{

	if (euid != u->u_pcred->p_ruid && euid != u->u_pcred->p_svuid && !suser())
		return (u->u_error);
	/*
	 * Everything's okay, do it.
	 */
	u->u_ucred->cr_uid = euid;
	u->u_acflag |= ASUGID;
	return (u->u_error = 0);
}

int
setgid()
{
	struct a {
		gid_t gid;
	} *uap = (struct a *)u->u_ap;
	
	return(_setgid(uap->gid));
}

int 
_setgid(gid)
	register gid_t gid;
{
	if (gid != u->u_pcred->p_rgid && !suser())
		return (u->u_error);	/* XXX */
	u->u_ucred->cr_groups[0] = gid;		/* effective gid is u_groups[0] */
	u->u_pcred->p_rgid = gid;
	u->u_pcred->p_svgid = gid;
	//u.u_acflag |= ASUGID;
	return (u->u_error = 0);
}

int
setegid()
{
	struct a {
		gid_t egid;
	} *uap = (struct a *)u->u_ap;

	return(_setegid(uap->egid));
}

int
_setegid(egid)
	register gid_t egid;
{
	if (egid != u->u_pcred->p_rgid && egid != u->u_pcred->p_svgid && !suser())
		return (u->u_error);
	u->u_ucred->cr_groups[0] = egid;
	u->u_acflag |= ASUGID;
	return (u->u_error = 0);
}

int
_setsid()
{
	register struct a {
		pid_t 	pid;
	}*uap = (struct a *) u->u_ap;

	return(_setsid(uap->pid));
}

int
setsid(pid)
	register pid_t pid;
{
	register struct proc *p;

	if(u->u_procp == pfind(pid)) {
		u->u_procp->p_pid = pid;
		p = u->u_procp;

		if(p->p_pgid == p->p_pid || pgfind(p->p_pid)) {
			return (u->u_error = EPERM);
		} else {
			(void)enterpgrp(p, p->p_pid, 1);
			return (u->u_error = 0);
		}
	} else {
		return (u->u_error = EPERM);
	}

	p->p_ucred;
}

int
setpgid()
{
	register struct a {
			pid_t 	pid;
			pid_t	pgid;
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
		int	ruid;
		int	euid;
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
		int rgid;
		int egid;
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
