/*
 * Thread Control Block:
 * Threads work nearly identically the same as proc in structure and operation.
 * Threads however cannot create child threads, this is done through proc. Threads have the same creds as the process
 * which created it.
 * Thread groups are linked to there proc group and cannot exceed the limit of a proc group and M:N ratio.
 * Example: If the M:N ratio (1:2) and prgp limit is 4. Then a Thread group limit would be 8 (2 Threads per Proc).
 *
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
struct tgrp *tgfind(tid_t);				/* Find Thread group by id. */

#endif /* SYS_TCB_H_ */
