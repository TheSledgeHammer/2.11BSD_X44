/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Common KThread & UThread functions */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <devel/sys/kthread.h>
#include <devel/libuthread/uthread.h>

struct tidhashhead *tidhashtbl;
u_long tidhash;
struct tgrphashhead *tgrphashtbl;
u_long tgrphash;

int	maxthread;// = NTHREAD;

void
threadinit()
{
	ktqinit();
	utqinit();
	tidhashtbl = hashinit(maxthread / 4, M_PROC, &tidhash);
	tgrphashtbl = hashinit(maxthread / 4, M_PROC, &tgrphash);
}

/*
 * init the kthread queues
 */
void
ktqinit()
{
	register struct kthread *kt;

	freekthread = NULL;
	for (kt = kthreadNKTHREAD; --kt > kthread0; freekthread = kt)
		kt->kt_nxt = freekthread;

	allkthread = kt;
	kt->kt_nxt = NULL;
	kt->kt_prev = &allkthread;

	zombkthread = NULL;
}

/*
 * init the uthread queues
 */
void
utqinit()
{
	register struct uthread *ut;

	freeuthread = NULL;
	for (ut = uthreadNUTHREAD; --ut > uthread0; freeuthread = ut)
		ut->ut_nxt = freeuthread;

	alluthread = ut;
	ut->ut_nxt = NULL;
	ut->ut_prev = &alluthread;

	zombuthread = NULL;
}

/*
 * Locate a thread group by number
 */
struct pgrp *
tgfind(pgid)
	register pid_t pgid;
{
	register struct pgrp *tgrp;
	for (tgrp = tgrphash[TIDHASH(pgid)]; tgrp != NULL; tgrp = tgrp->pg_hforw) {
		if (tgrp->pg_id == pgid) {
			return (tgrp);
		}
	}
	return (NULL);
}

/*
 * delete a thread group
 */
void
tgdelete(tgrp)
	register struct pgrp *tgrp;
{
	register struct pgrp **tgp = &tgrphash[TIDHASH(tgrp->pg_id)];

	if (tgrp->pg_session->s_ttyp != NULL &&
		tgrp->pg_session->s_ttyp->t_pgrp == tgrp)
		tgrp->pg_session->s_ttyp->t_pgrp = NULL;
	for (; *tgp; tgp = &(*tgp)->pg_hforw) {
		if (*tgp == tgrp) {
			*tgp = tgrp->pg_hforw;
			break;
		}
	}
#ifdef DIAGNOSTIC
	if (pgp == NULL)
		panic("tgdelete: can't find pgrp on hash chain");
#endif
	if (--tgrp->pg_session->s_count == 0)
		FREE(tgrp, M_PGRP);
	FREE(tgrp, M_PGRP);
}
