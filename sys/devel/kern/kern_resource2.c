/*-
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)kern_resource.c	8.8 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <vm/include/vm.h>

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
