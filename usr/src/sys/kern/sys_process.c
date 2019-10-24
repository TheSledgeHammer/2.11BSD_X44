/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_process.c	1.2 (2.11BSD) 1999/9/5
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/inode.h>
#include <vm/vm.h>
#include <sys/ptrace.h>

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct
{
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
} ipc;

/*
 * sys-trace system call.
 */
void
ptrace()
{
	register struct proc *p;
	register struct a {
		int	req;
		int	pid;
		int	*addr;
		int	data;
	} *uap;

	uap = (struct a *)u->u_ap;
	if (uap->req <= 0) {
		u->u_procp->p_flag |= P_TRACED;
		return;
	}
	p = pfind(uap->pid);
	if (p == 0 || p->p_stat != SSTOP || p->p_ppid != u->u_procp->p_pid ||
	    !(p->p_flag & P_TRACED)) {
		u->u_error = ESRCH;
		return;
	}
	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, PZERO);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag &= ~P_WAITED;
	setrun(p);
	while (ipc.ip_req > 0)
		sleep((caddr_t)&ipc, PZERO);
	u->u_r.r_val1 = (short)ipc.ip_data;
	if (ipc.ip_req < 0)
		u->u_error = EIO;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}
