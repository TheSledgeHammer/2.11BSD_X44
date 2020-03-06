/*
 * thread.h
 *
 *  Created on: 6 Mar 2020
 *      Author: marti
 */
 /* Thread Control Block */
#ifndef SYS_TCB_H_
#define SYS_TCB_H_

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
#define TSSLEEP	1
#define TSWAIT	2
#define TSRUN	3
#define TSIDL	4
#define TSREADY	5
#define TSSTART	6
#define TSSTOP	7

#define	TIDHSZ			16
#define	TIDHASH(tid)	(&tidhashtbl[(tid) & tid_hash & (TIDHSZ * ((tid) + tid_hash) - 1)])
u_long 					tid_hash;

#define TGRPHASH(tgid)					(&tgrphashtbl[(tgid) & tgrphash])
extern 	LIST_HEAD(tgrphashhead, tgrp) 	*tgrphashtbl;
u_long 									tgrphash;

extern int tidhashmask;					/* In param.c. */

void 		threadinit(void);
struct tgrp *tgfind(tid_t);				/* Find Thread group by id. */

#endif /* SYS_TCB_H_ */
