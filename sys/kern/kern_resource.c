/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_resource.c	1.4 (2.11BSD GTE) 1997/2/14
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <vm/include/vm.h>

/*
 * Resource controls and accounting.
 */
int
getpriority()
{
	register struct getpriority_args {
		syscallarg(int)	which;
		syscallarg(int)	who;
	} *uap = (struct getpriority_args *)u.u_ap;
	register struct proc *p;
	register int low = PRIO_MAX + 1;

	switch (SCARG(uap, which)) {

	case PRIO_PROCESS:
		if (SCARG(uap, who) == 0)
			p = u.u_procp;
		else
			p = pfind(SCARG(uap, who));
		if (p == 0)
			break;
		low = p->p_nice;
		break;

	case PRIO_PGRP:
		if (SCARG(uap, who) == 0)
			SCARG(uap, who) = u.u_procp->p_pgrp;
		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_pgrp == SCARG(uap, who) && p->p_nice < low)
				low = p->p_nice;
		}
		break;

	case PRIO_USER:
		if (SCARG(uap, who) == 0)
			SCARG(uap, who) = u.u_uid;
		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_uid == SCARG(uap, who) && p->p_nice < low)
				low = p->p_nice;
		}
		break;

	default:
		u.u_error = EINVAL;
		return (EINVAL);
	}
	if (low == PRIO_MAX + 1) {
		u.u_error = ESRCH;
		return (ESRCH);
	}
	u.u_r.r_val1 = low;
	return (0);
}

int
setpriority()
{
	register struct setpriority_args {
		syscallarg(int)	which;
		syscallarg(int)	who;
		syscallarg(int)	prio;
	} *uap = (struct setpriority_args *)u.u_ap;
	register struct proc *p;
	register int found = 0;

	switch (SCARG(uap, which)) {

	case PRIO_PROCESS:
		if (SCARG(uap, who) == 0)
			p = u.u_procp;
		else
			p = pfind(SCARG(uap, who));
		if (p == 0)
			break;
		donice(p, SCARG(uap, prio));
		found++;
		break;

	case PRIO_PGRP:
		if (SCARG(uap, who) == 0)
			SCARG(uap, who) = u.u_procp->p_pgrp;
		for (p = allproc; p != NULL; p = p->p_nxt)
			if (p->p_pgrp == SCARG(uap, who)) {
				donice(p, SCARG(uap, prio));
				found++;
			}
		break;

	case PRIO_USER:
		if (SCARG(uap, who) == 0)
			SCARG(uap, who) = u.u_uid;
		for (p = allproc; p != NULL; p = p->p_nxt)
			if (p->p_uid == SCARG(uap, who)) {
				donice(p, SCARG(uap, prio));
				found++;
			}
		break;

	default:
		u.u_error = EINVAL;
		return (EINVAL);
	}
	if (found == 0)
		u.u_error = ESRCH;
		return (ESRCH);

	return (u.u_error);
}

static void
donice(p, n)
	register struct proc *p;
	register int n;
{
	if (u.u_uid && u.u_pcred->p_ruid &&
	    u.u_uid != p->p_uid && u.u_pcred->p_ruid != p->p_uid) {
		u.u_error = EPERM;
		return;
	}
	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	if (n < p->p_nice && !suser()) {
		u.u_error = EACCES;
		return;
	}
	p->p_nice = n;
}

int
setrlimit()
{
	register struct setrlimit_args {
		syscallarg(u_int) which;
		syscallarg(struct rlimit *) lim;
	} *uap = (struct setrlimit_args *)u.u_ap;
	struct rlimit alim;
	register struct rlimit *alimp;
	extern unsigned int maxdmap;

	if (SCARG(uap, which) >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return (EINVAL);
	}
	alimp = &u.u_rlimit[SCARG(uap, which)];
	u.u_error = copyin((caddr_t)SCARG(uap, lim), (caddr_t)&alim, sizeof (struct rlimit));
	if (u.u_error)
		return (u.u_error);
	if (SCARG(uap, which) == RLIMIT_CPU) {
		/*
		 * 2.11 stores RLIMIT_CPU as ticks to keep from making
		 * hardclock() do long multiplication/division.
		 */
		if (alim.rlim_cur >= RLIM_INFINITY / hz)
			alim.rlim_cur = RLIM_INFINITY;
		else
			alim.rlim_cur = alim.rlim_cur * hz;
		if (alim.rlim_max >= RLIM_INFINITY / hz)
			alim.rlim_max = RLIM_INFINITY;
		else
			alim.rlim_max = alim.rlim_max * hz;
	}
	if (alim.rlim_cur > alimp->rlim_max || alim.rlim_max > alimp->rlim_max) {
		if (u.u_error == suser()) {
			return (u.u_error);
		}
	}

	//*alimp = alim;
	return (dosetrlimit(u.u_procp, SCARG(uap, which), alim, alimp));
}

int
dosetrlimit(p, which, limp, alimp)
	struct proc *p;
	u_int which;
	struct rlimit *limp, *alimp;
{
	switch (which) {
	case RLIMIT_DATA:
		if (limp->rlim_cur > maxdmap)
			limp->rlim_cur = maxdmap;
		if (limp->rlim_max > maxdmap)
			limp->rlim_max = maxdmap;
		break;

	case RLIMIT_STACK:
		if (limp->rlim_cur > maxdmap)
			limp->rlim_cur = maxdmap;
		if (limp->rlim_max > maxdmap)
			limp->rlim_max = maxdmap;
		/*
		 * Stack is allocated to the max at exec time with only
		 * "rlim_cur" bytes accessible.  If stack limit is going
		 * up make more accessible, if going down make inaccessible.
		 */
		if (limp->rlim_cur != alimp->rlim_cur) {
			vm_offset_t addr;
			vm_size_t size;
			vm_prot_t prot;

			if (limp->rlim_cur > alimp->rlim_cur) {
				prot = VM_PROT_ALL;
				size = limp->rlim_cur - alimp->rlim_cur;
				addr = USRSTACK - limp->rlim_cur;
			} else {
				prot = VM_PROT_NONE;
				size = alimp->rlim_cur - limp->rlim_cur;
				addr = USRSTACK - alimp->rlim_cur;
			}
			addr = trunc_page(addr);
			size = round_page(size);
			(void) vm_map_protect(&p->p_vmspace->vm_map, addr, addr + size, prot, FALSE);
		}
		break;

	case RLIMIT_NOFILE:
		if (limp->rlim_cur > maxfiles)
			limp->rlim_cur = maxfiles;
		if (limp->rlim_max > maxfiles)
			limp->rlim_max = maxfiles;
		break;

	case RLIMIT_NPROC:
		if (limp->rlim_cur > maxproc)
			limp->rlim_cur = maxproc;
		if (limp->rlim_max > maxproc)
			limp->rlim_max = maxproc;
		break;
	}
	*alimp = *limp;
	return (0);
}

int
getrlimit()
{
	register struct getrlimit_args {
		syscallarg(u_int) 			which;
		syscallarg(struct rlimit *) rlp;
	} *uap = (struct getrlimit_args *)u.u_ap;

	if (SCARG(uap, which) >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return (EINVAL);
	}
	if (SCARG(uap, which) == RLIMIT_CPU) {
		struct rlimit alim;

		alim = u.u_rlimit[SCARG(uap, which)];
		if (alim.rlim_cur != RLIM_INFINITY)
			alim.rlim_cur = alim.rlim_cur / hz;
		if (alim.rlim_max != RLIM_INFINITY)
			alim.rlim_max = alim.rlim_max / hz;
		u.u_error = copyout((caddr_t) & alim, (caddr_t) SCARG(uap, rlp),
				sizeof(struct rlimit));
	} else
		u.u_error = copyout((caddr_t) & u.u_rlimit[SCARG(uap, which)],
				(caddr_t) SCARG(uap, rlp), sizeof(struct rlimit));

	return (u.u_error);
}

int
getrusage()
{
	register struct getrusage_args {
		syscallarg(int) who;
		syscallarg(struct rusage *) rusage;
	} *uap = (struct getrusage_args *)u.u_ap;
	register struct k_rusage *rup;
	struct rusage ru;

	switch (SCARG(uap, who)) {

	case RUSAGE_SELF:
		rup = &u.u_ru;
		break;

	case RUSAGE_CHILDREN:
		rup = &u.u_cru;
		break;

	default:
		u.u_error = EINVAL;
		return (EINVAL);
	}
	rucvt(&ru,rup);
	u.u_error = copyout((caddr_t)&ru, (caddr_t)SCARG(uap, rusage), sizeof (struct rusage));
	return (u.u_error);
}

void
ruadd(ru, ru2)
	struct k_rusage *ru, *ru2;
{
	register long *ip, *ip2;
	register int i;
	struct rusage *kru, *kru2;

	/*
	 * since the kernel timeval structures are single longs,
	 * fold them into the loop.
	 */
	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (kru->ru_maxrss < kru2->ru_maxrss)
		kru->ru_maxrss = kru2->ru_maxrss;
	ip = &ru->k_ru_first; ip2 = &ru2->k_ru_first;
	for (i = &ru->k_ru_last - &ru->k_ru_first; i >= 0; i--)
		*ip++ += *ip2++;
}

/*
 * Convert an internal kernel rusage structure into a `real' rusage structure.
 */
void
rucvt(rup, krup)
	register struct rusage		*rup;
	register struct k_rusage	*krup;
{
	bzero((caddr_t)rup, sizeof(*rup));
	rup->ru_utime.tv_sec   = krup->ru_utime / hz;
	rup->ru_utime.tv_usec  = (krup->ru_utime % hz) * mshz;
	rup->ru_stime.tv_sec   = krup->ru_stime / hz;
	rup->ru_stime.tv_usec  = (krup->ru_stime % hz) * mshz;
	rup->ru_ovly = krup->ru_ovly;
	rup->ru_nswap = krup->ru_nswap;
	rup->ru_inblock = krup->ru_inblock;
	rup->ru_oublock = krup->ru_oublock;
	rup->ru_msgsnd = krup->ru_msgsnd;
	rup->ru_msgrcv = krup->ru_msgrcv;
	rup->ru_nsignals = krup->ru_nsignals;
	rup->ru_nvcsw = krup->ru_nvcsw;
	rup->ru_nivcsw = krup->ru_nivcsw;
}

struct plimit *
limcopy(lim)
	struct plimit *lim;
{
	register struct plimit *copy;

	rmalloc(copy, sizeof(struct plimit));
	bcopy(lim->pl_rlimit, copy->pl_rlimit, sizeof(struct rlimit) * RLIM_NLIMITS);
	copy->p_lflags = 0;
	copy->p_refcnt = 1;
	return (copy);
}

/*
 * Transform the running time and tick information in proc p into user,
 * system, and interrupt time usage.
 */
void
calcru(p, up, sp, ip)
	register struct proc *p;
	register struct timeval *up;
	register struct timeval *sp;
	register struct timeval *ip;
{
	register u_quad_t u, st, ut, it, tot;
	register u_long sec, usec;
	register int s;
	struct timeval tv;

	s = splstatclock();
	st = p->p_sticks;
	ut = p->p_uticks;
	it = p->p_iticks;
	splx(s);

	tot = st + ut + it;
	if (tot == 0) {
		up->tv_sec = up->tv_usec = 0;
		sp->tv_sec = sp->tv_usec = 0;
		if (ip != NULL)
			ip->tv_sec = ip->tv_usec = 0;
		return;
	}

	sec = p->p_rtime.tv_sec;
	usec = p->p_rtime.tv_usec;
	if (p == curproc) {
		/*
		 * Adjust for the current time slice.  This is actually fairly
		 * important since the error here is on the order of a time
		 * quantum, which is much greater than the sampling error.
		 */
		microtime(&tv);
		sec += tv.tv_sec - runtime.tv_sec;
		usec += tv.tv_usec - runtime.tv_usec;
	}
	u = sec * 1000000 + usec;
	st = (u * st) / tot;
	sp->tv_sec = st / 1000000;
	sp->tv_usec = st % 1000000;
	ut = (u * ut) / tot;
	up->tv_sec = ut / 1000000;
	up->tv_usec = ut % 1000000;
	if (ip != NULL) {
		it = (u * it) / tot;
		ip->tv_sec = it / 1000000;
		ip->tv_usec = it % 1000000;
	}
}
