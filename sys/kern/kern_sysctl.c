/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
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
 *	@(#)kern_sysctl.c	8.4.11 (2.11BSD) 1999/8/11
 */

/*
 * sysctl system call.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/boot.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/map.h>
#include <sys/sysctl.h>

#include <vm/include/vm.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

sysctlfn 		kern_sysctl;
sysctlfn 		hw_sysctl;
#ifdef DEBUG
sysctlfn 		debug_sysctl;
#endif
extern sysctlfn vm_sysctl;
extern sysctlfn vfs_sysctl;
extern sysctlfn net_sysctl;
extern sysctlfn cpu_sysctl;

/*
 * Locking and stats
 */
static struct sysctl_lock {
	int	sl_lock;
	int	sl_want;
	int	sl_locked;
} memlock;

int
__sysctl()
{
	register struct sysctl_args {
		syscallarg(int *) name;
		syscallarg(u_int) namelen;
		syscallarg(void	*) old;
		syscallarg(size_t *) oldlenp;
		syscallarg(void	*) new;
		syscallarg(size_t) newlen;
	} *uap = (struct sysctl_args *)u.u_ap;

	int error;
	u_int savelen, oldlen = 0;
	sysctlfn *fn;
	int name[CTL_MAXNAME];

	if (SCARG(uap, new) != NULL && !suser())
		return (u.u_error);	/* XXX */
	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (SCARG(uap, namelen) > CTL_MAXNAME || SCARG(uap, namelen) < 2)
		return (u.u_error = EINVAL);
	if (error == copyin(SCARG(uap, name), &name, SCARG(uap, namelen) * sizeof(int)))
		return (u.u_error = error);

	switch (name[0]) {
	case CTL_KERN:
		fn = kern_sysctl;
		break;
	case CTL_HW:
		fn = hw_sysctl;
		break;
	case CTL_VM:
		fn = vm_sysctl;
		break;
#ifdef INET
	case CTL_NET:
		fn = net_sysctl;
		break;
#endif
	case CTL_VFS:
		fn = vfs_sysctl;
		break;
	case CTL_MACHDEP:
		fn = cpu_sysctl;
		break;
#ifdef DEBUG
	case CTL_DEBUG:
		fn = debug_sysctl;
		break;
#endif
#ifdef DDB
	case CTL_DDB:
		fn = ddb_sysctl;
		break;
#endif
	default:
		return (u.u_error = EOPNOTSUPP);
	}

	if (SCARG(uap, oldlenp) &&
	    (error = copyin(SCARG(uap, oldlenp), &oldlen, sizeof(oldlen))))
		return (u.u_error = error);
	if (SCARG(uap, old) != NULL) {
		while (memlock.sl_lock) {
			memlock.sl_want = 1;
			sleep((caddr_t)&memlock, PRIBIO+1);
			memlock.sl_locked++;
		}
		memlock.sl_lock = 1;
		savelen = oldlen;
	}
	error = (*fn)(name + 1, SCARG(uap, namelen) - 1, SCARG(uap, old), &oldlen, SCARG(uap, new), SCARG(uap, newlen));
	if (SCARG(uap, old) != NULL) {
		memlock.sl_lock = 0;
		if (memlock.sl_want) {
			memlock.sl_want = 0;
			wakeup((caddr_t)&memlock);
		}
	}
	if (error)
		return (u.u_error = error);
	if (SCARG(uap, oldlenp)) {
		error = copyout(&oldlen, SCARG(uap, oldlenp), sizeof(oldlen));
		if (error)
			return(u.u_error = error);
	}
	u.u_r.r_val1 = oldlen;
	return (0);
}

/*
 * Attributes stored in the kernel.
 */
char hostname[MAXHOSTNAMELEN];
int hostnamelen;
char domainname[MAXHOSTNAMELEN];
int domainnamelen;
long hostid;
//int securelevel;
char kernelname[MAXPATHLEN] = PATH_KERNEL;	/* XXX bloat */
int maxpartitions = MAXPARTITIONS;
int raw_part = RAW_PART;
char osversion[0];
//char osrelease[];
long osrevision[0];

/*
 * kernel related system variables.
 */
int
kern_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	int error, level;
	u_long longhostid;

	/* all sysctl names at this level are terminal */
	if (namelen != 1 && !(name[0] == KERN_PROC || name[0] == KERN_PROF))
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case KERN_OSTYPE:
			return (sysctl_rdstring(oldp, oldlenp, newp, ostype));
	case KERN_OSRELEASE:
			return (sysctl_rdstring(oldp, oldlenp, newp, osrelease));
	case KERN_OSREV:
		return (sysctl_rdlong(oldp, oldlenp, newp, *osrevision));
	case KERN_OSVERSION:
		return (sysctl_rdstring(oldp, oldlenp, newp, osversion));
	case KERN_VERSION:
		return (sysctl_rdstring(oldp, oldlenp, newp, version));
	case KERN_MAXVNODES:
		return(sysctl_rdint(oldp, oldlenp, newp, desiredvnodes));
	case KERN_MAXPROC:
		return (sysctl_rdint(oldp, oldlenp, newp, maxproc));
	case KERN_MAXFILES:
		return (sysctl_rdint(oldp, oldlenp, newp, maxfiles));
	case KERN_ARGMAX:
		return (sysctl_rdint(oldp, oldlenp, newp, ARG_MAX));
	case KERN_SECURELVL:
		level = securelevel;
		if ((error = sysctl_int(oldp, oldlenp, newp, newlen, &level)) || newp == NULL) {
			return (error);
		}
		if (level < securelevel && u.u_procp->p_pid != 1) {
			return (EPERM);
		}
		securelevel = level;
		return (0);
	case KERN_HOSTNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen, hostname, sizeof(hostname));
		if (newp && !error) {
			hostnamelen = newlen;
		}
		return (error);
	case KERN_HOSTID:
		longhostid = hostid;
		error =  sysctl_long(oldp, oldlenp, newp, newlen, &longhostid);
		hostid = longhostid;
		return (error);
	case KERN_CLOCKRATE:
		return (sysctl_clockrate(oldp, oldlenp));
	case KERN_BOOTTIME:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &boottime, sizeof(struct timeval)));
	case KERN_VNODE:
		return (sysctl_vnode(oldp, oldlenp, u.u_procp));
	case KERN_PROC:
		return (sysctl_doproc(name + 1, namelen - 1, oldp, oldlenp));
	case KERN_FILE:
		return (sysctl_file(oldp, oldlenp));
#ifdef GPROF
	case KERN_PROF:
		return (sysctl_doprof(name + 1, namelen - 1, oldp, oldlenp, newp, newlen));
#endif
	case KERN_NGROUPS:
		return (sysctl_rdint(oldp, oldlenp, newp, NGROUPS_MAX));
	case KERN_JOB_CONTROL:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_POSIX1:
		return (sysctl_rdint(oldp, oldlenp, newp, _POSIX_VERSION));
	case KERN_SAVED_IDS:
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
	case KERN_BOOTFILE:
		return (sysctl_string(oldp, oldlenp, newp, newlen, kernelname, sizeof(kernelname)));
	case KERN_MAXPARTITIONS:
		return (sysctl_rdint(oldp, oldlenp, newp, maxpartitions));
	case KERN_RAWPARTITION:
		return (sysctl_rdint(oldp, oldlenp, newp, raw_part));
	case KERN_TIMECOUNTER:
		return (sysctl_timecounter(name + 1, namelen - 1, oldp, oldlenp, newp, newlen));
	case KERN_DOMAINNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen, domainname, sizeof(domainname));
		if (newp && !error) {
			domainnamelen = newlen;
		}
		return (error);
	case KERN_URND:
		return (sysctl_urandom(oldp, oldlenp, newp));
#ifndef _KERNEL
	case KERN_ARND:
		return (sysctl_arandom(oldp, oldlenp, newp));
#endif
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * hardware related system variables.
 */
int
hw_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	extern char machine[], cpu_model[128];

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		    /* overloaded */

	switch (name[0]) {
	case HW_MACHINE:
		return (sysctl_rdstring(oldp, oldlenp, newp, machine));
	case HW_MODEL:
		return (sysctl_rdstring(oldp, oldlenp, newp, cpu_model));
	case HW_NCPU:
		return (sysctl_rdint(oldp, oldlenp, newp, NCPUS));	/* XXX */
	case HW_BYTEORDER:
		return (sysctl_rdint(oldp, oldlenp, newp, BYTE_ORDER));
	case HW_PHYSMEM:
		return (sysctl_rdlong(oldp, oldlenp, newp, ctob((long)physmem)));
	case HW_USERMEM:
		return (sysctl_rdlong(oldp, oldlenp, newp, ctob((long)physmem - freemem)));
	case HW_PAGESIZE:
		return (sysctl_rdint(oldp, oldlenp, newp, PAGE_SIZE));
	case HW_DISKNAMES:
		return (sysctl_disknames(oldp, oldlenp));
	case HW_CNMAGIC:
		return (sysctl_cnmagic(oldp, oldlenp, newp, newlen));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

#ifdef DEBUG
/*
 * Debugging related system variables.
 */
struct ctldebug debug0, debug1, debug2, debug3, debug4;
struct ctldebug debug5, debug6, debug7, debug8, debug9;
struct ctldebug debug10, debug11, debug12, debug13, debug14;
struct ctldebug debug15, debug16, debug17, debug18, debug19;

static struct ctldebug *debugvars[CTL_DEBUG_MAXID] = {
	&debug0, &debug1, &debug2, &debug3, &debug4,
	&debug5, &debug6, &debug7, &debug8, &debug9,
	&debug10, &debug11, &debug12, &debug13, &debug14,
	&debug15, &debug16, &debug17, &debug18, &debug19,
};
int
debug_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	struct ctldebug *cdp;

	/* all sysctl names at this level are name and field */
	if (namelen != 2)
		return (ENOTDIR);		/* overloaded */
	cdp = debugvars[name[0]];
	if (cdp->debugname == 0)
		return (EOPNOTSUPP);
	switch (name[1]) {
	case CTL_DEBUG_NAME:
		return (sysctl_rdstring(oldp, oldlenp, newp, cdp->debugname));
	case CTL_DEBUG_VALUE:
		return (sysctl_int(oldp, oldlenp, newp, newlen, cdp->debugvar));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* DEBUG */

/*
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
int
sysctl_int(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	int *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp && newlen != sizeof(int))
		return (EINVAL);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout(valp, oldp, sizeof(int));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(int));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdint(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	int val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(int));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an long-valued sysctl function.
 */
int
sysctl_long(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	long *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(long))
		return (ENOMEM);
	if (newp && newlen != sizeof(long))
		return (EINVAL);
	*oldlenp = sizeof(long);
	if (oldp)
		error = copyout(valp, oldp, sizeof(long));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(long));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdlong(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	long val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(long))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(long);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(long));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a string-valued sysctl function.
 */
int
sysctl_string(oldp, oldlenp, newp, newlen, str, maxlen)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	char *str;
	int maxlen;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen >= maxlen)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = copyout(str, oldp, len);
	}
	if (error == 0 && newp) {
		error = copyin(newp, str, newlen);
		str[newlen] = 0;
	}
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdstring(oldp, oldlenp, newp, str)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	char *str;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = copyout(str, oldp, len);
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_struct(oldp, oldlenp, newp, newlen, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	void *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen > len)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = copyout(sp, oldp, len);
	}
	if (error == 0 && newp)
		error = copyin(newp, sp, len);
	return (error);
}

/*
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
int
sysctl_rdstruct(oldp, oldlenp, newp, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp, *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = copyout(sp, oldp, len);
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an quad-valued sysctl function.
 */
int
sysctl_quad(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	quad_t *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(quad_t))
		return (ENOMEM);
	if (newp && newlen != sizeof(quad_t))
		return (EINVAL);
	*oldlenp = sizeof(quad_t);
	if (oldp)
		error = copyout(valp, oldp, sizeof(quad_t));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(quad_t));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdquad(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	quad_t val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(quad_t))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(quad_t);
	if (oldp)
		error = copyout((caddr_t) &val, oldp, sizeof(quad_t));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an bool-valued sysctl function.
 */
int
sysctl_bool(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	bool_t *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(bool_t))
		return (ENOMEM);
	if (newp && newlen != sizeof(bool_t))
		return (EINVAL);
	*oldlenp = sizeof(bool_t);
	if (oldp)
		error = copyout(valp, oldp, sizeof(bool_t));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(bool_t));
	return (error);
}

/*
 * As above, but read-only.
 */
int
sysctl_rdbool(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	bool_t val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(bool_t))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(bool_t);
	if (oldp)
		error = copyout((caddr_t) &val, oldp, sizeof(bool_t));
	return (error);
}

/*
 * Get file structures.
 */
int
sysctl_file(where, sizep)
	char *where;
	size_t *sizep;
{
	int buflen, error;
	register struct file *fp;
	struct	file *fpp;
	char *start = where;
	register int i;

	buflen = *sizep;
	if (where == NULL) {
#define	FPTRSZ	sizeof (struct file *)
#define	FILESZ	sizeof (struct file)
#define	FILEHD	sizeof (filehead)
		/*
		 * overestimate by 5 files
		 */
		*sizep = FILEHD + (nfiles + 10) * (FILESZ + FPTRSZ);
		return (0);
	}

	/*
	 * array of extended file structures: first the address then the
	 * file structure.
	 */
	for (fp = LIST_FIRST(&filehead); fp != NULL; fp = LIST_NEXT(fp, f_list)) {
		if (fp->f_count == 0)
			continue;
		if (buflen < (FPTRSZ + FILESZ)) {
			*sizep = where - start;
			return (ENOMEM);
		}
		fpp = fp;
		if ((error = copyout(&fpp, where, FPTRSZ)) ||
			(error = copyout(fp, where + FPTRSZ, FILESZ)))
			return (error);
		buflen -= (FPTRSZ + FILESZ);
		where += (FPTRSZ + FILESZ);
	}
	*sizep = where - start;
	return (0);
}

#ifdef notyet
/*
 * This one is in kern_clock.c in 4.4 but placed here for the reasons
 * given earlier (back around line 367).
*/
/*
 * Return information about system clocks.
 */
int
sysctl_clockrate(where, sizep)
	char *where;
	size_t *sizep;
{
	struct	clockinfo clkinfo;

	/*
	 * Construct clockinfo structure.
	*/
	clkinfo.hz = hz;
	clkinfo.tick = mshz;
	clkinfo.profhz = profhz;
	clkinfo.stathz = stathz ? stathz : hz;
	return (sysctl_rdstruct(where, sizep, NULL, &clkinfo, sizeof (clkinfo)));
}
#endif
/*
 * try over estimating by 5 procs
 */
#define KERN_PROCSLOP	(5 * sizeof (struct kinfo_proc))
int
sysctl_doproc(name, namelen, where, sizep)
	int *name;
	u_int namelen;
	char *where;
	size_t *sizep;
{
	register struct proc *p;

	register struct kinfo_proc *dp = (struct kinfo_proc *)where;
	int needed = 0;
	int buflen = where != NULL ? *sizep : 0;
	int doingzomb;
	struct eproc eproc;
	int error = 0;
	dev_t ttyd;
	uid_t ruid;
	struct tty *ttyp;

	if (namelen != 2 && !(namelen == 1 && name[0] == KERN_PROC_ALL))
		return (EINVAL);
	p = LIST_FIRST(&allproc);
	doingzomb = 0;
again:
	for (; p != NULL; p = LIST_NEXT(p, p_list)) {
		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;
		/*
		 * TODO - make more efficient (see notes below).
		 * do by session.
		 */
		switch (name[0]) {

		case KERN_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp->pg_id != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_TTY:
			if ((p->p_flag & P_CONTROLT) == 0 || p->p_session->s_ttyp == NULL
					|| p->p_session->s_ttyp->t_dev != (dev_t) name[1])
				continue;
			break;

		case KERN_PROC_UID:
			if (p->p_ucred->cr_uid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_RUID:
			if (p->p_cred->p_ruid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_ALL:
			break;
		default:
			return(EINVAL);
		}
		if (buflen >= sizeof(struct kinfo_proc)) {
			fill_eproc(p, &eproc);
			if (error == copyout((caddr_t)p, &dp->kp_proc,
			    sizeof(struct proc)))
				return (error);
			if (error == copyout((caddr_t)&eproc, &dp->kp_eproc,
			    sizeof(eproc)))
				return (error);
			dp++;
			buflen -= sizeof(struct kinfo_proc);
		}
		needed += sizeof(struct kinfo_proc);
	}
	if (doingzomb == 0) {
		p = LIST_FIRST(&zombproc);
		doingzomb++;
		goto again;
	}
	if (where != NULL) {
		*sizep = (caddr_t)dp - where;
		if (needed > *sizep)
			return (ENOMEM);
	} else {
		needed += KERN_PROCSLOP;
		*sizep = needed;
	}
	return (0);
}

/*
 * Fill in an eproc structure for the specified process.  Slightly
 * inefficient because we have to access the u area again for the
 * information not kept in the proc structure itself.  Can't afford
 * to expand the proc struct so we take a slight speed hit here.
 */
void
fill_eproc(p, ep)
	register struct proc *p;
	register struct eproc *ep;
{
	struct	tty	*ttyp;

	ep->e_paddr = p;
	ep->e_sess = p->p_pgrp->pg_session;
	ep->e_pcred = *p->p_cred;
	ep->e_ucred = *p->p_ucred;
	if (p->p_stat == SIDL || p->p_stat == SZOMB) {
		ep->e_vm->vm_rssize = 0;
		ep->e_vm->vm_tsize = 0;
		ep->e_vm->vm_dsize = 0;
		ep->e_vm->vm_ssize = 0;
	} else {
		register struct vmspace *vm = p->p_vmspace;
#ifdef pmap_resident_count
		ep->e_vm->vm_rssize = pmap_resident_count(&vm->vm_pmap); /*XXX*/
#else
		ep->e_vm->vm_rssize = vm->vm_rssize;
#endif
		ep->e_vm->vm_tsize = vm->vm_tsize;
		ep->e_vm->vm_dsize = vm->vm_dsize;
		ep->e_vm->vm_ssize = vm->vm_ssize;
	}
	if (p->p_pptr) {
		ep->e_ppid = p->p_pptr->p_pid;
	} else {
		ep->e_ppid = 0;
	}
	ep->e_pgid = p->p_pgrp->pg_id;
	ep->e_jobc = p->p_pgrp->pg_jobc;
	if ((p->p_flag & P_CONTROLT) && (ttyp = ep->e_sess->s_ttyp)) {
		ep->e_tdev = ttyp->t_dev;
		ep->e_tpgid = ttyp->t_pgrp ? ttyp->t_pgrp->pg_id : NO_PID;
		ep->e_tsess = ttyp->t_session;
	} else {
		ep->e_tdev = NODEV;
	}
	ep->e_flag = ep->e_sess->s_ttyvp ? EPROC_CTTY : 0;
	if (SESS_LEADER(p)) {
		ep->e_flag |= EPROC_SLEADER;
	}
	if (p->p_wmesg) {
		strncpy(ep->e_wmesg, p->p_wmesg, WMESGLEN);
	}
	ep->e_xsize = ep->e_xrssize = 0;
	ep->e_xccount = ep->e_xswrss = 0;
}
