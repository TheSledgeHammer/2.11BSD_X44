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

/*
 * Thread Control Block:
 * Thread tid's are a sub id of proc pid's		(Intended Goal)
 * Thread groups are a subgroup of proc groups. (Intended Goal)
 */

 /* Thread Control Block */
#ifndef SYS_TCB_H_
#define SYS_TCB_H_

#include <sys/proc.h>

/* put into types.h */
typedef long			tid_t;			/* thread id */

struct tgrp {
	struct	tgrp 		*tg_hforw;		/* Forward link in hash bucket. */
	struct	tgrp 		*tg_mem;		/* Pointer to tgrp members. */
	struct	session 	*tg_session;	/* Pointer to session. */
	tid_t				tg_id;			/* Thread Group id. */
	int					tg_jobc;		/* # threads qualifying tgrp for job control */
};

/* stat codes */
#define TSSLEEP	1		/* sleeping/ awaiting an event */
#define TSWAIT	2		/* waiting */
#define TSRUN	3		/* running */
#define TSIDL	4		/* intermediate state in process creation */
#define	TSZOMB	5		/* intermediate state in process termination */
#define TSSTOP	6		/* process being traced */
#define TSREADY	7		/* ready */
#define TSSTART	8		/* start */

#define	TIDHSZ			16
#define	TIDHASH(tid)	(&tidhashtbl[(tid) & tid_hash & (TIDHSZ * ((tid) + tid_hash) - 1)])
u_long 					tid_hash;

#define TGRPHASH(tgid)					(&tgrphashtbl[(tgid) & tgrphash])
extern 	LIST_HEAD(tgrphashhead, tgrp) 	*tgrphashtbl;
u_long 									tgrphash;

extern int tidhashmask;						/* In param.c. */

void 		threadinit(void);
struct tgrp *tgfind(tid_t);					/* Find Thread group by id. */

#endif /* SYS_TCB_H_ */
