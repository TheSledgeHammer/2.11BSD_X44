/*
 * Copyright (c) 1980, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ptrace.h	7.1 (Berkeley) 6/4/86
 */

#ifndef _PTRACE_
#define _PTRACE_

#define PT_TRACE_ME		0	/* child declares it's being traced */
#define PT_READ_I		1	/* read word in child's I space */
#define PT_READ_D		2	/* read word in child's D space */
#define PT_READ_U		3	/* read word in child's user structure */
#define PT_WRITE_I		4	/* write word in child's I space */
#define PT_WRITE_D		5	/* write word in child's D space */
#define PT_WRITE_U		6	/* write word in child's user structure */
#define PT_CONTINUE		7	/* continue the child */
#define PT_KILL			8	/* kill the child process */
#define PT_STEP			9	/* single step the child */
#define	PT_ATTACH		10	/* trace some running process */
#define	PT_DETACH		11	/* stop tracing a process */


#define	PT_FIRSTMACH	32	/* for machine-specific requests */
#include <machine/ptrace.h>	/* machine-specific requests, if any */

#ifdef KERNEL
void	proc_reparent (struct proc *child, struct proc *newparent);
#else /* !KERNEL */

#include <sys/cdefs.h>

__BEGIN_DECLS
int	ptrace (int _request, pid_t _pid, caddr_t _addr, int _data);
__END_DECLS

#endif /* !KERNEL */
#endif
