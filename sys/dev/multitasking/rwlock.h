/*
 * rwlock.h
 *
 *  Created on: 6 Mar 2020
 *      Author: marti
 */

#ifndef SYS_RWLOCK_H_
#define SYS_RWLOCK_H_

#include <sys/lock.h>

/* Reader Writers Lock */
struct rwlock {
    struct simplelock       *rwl_interlock;      /* lock on remaining fields */
    const char              *rwl_name;

    int					    rwl_sharecount;		/* # of accepted shared locks */
    int					    rwl_waitcount;		/* # of processes sleeping for lock */
    short				    rw_exclusivecount;	/* # of recursive exclusive locks */

    unsigned int		    rwl_flags;			/* see below */
    short				    rwl_prio;			/* priority at which to sleep */
    char				    *rwl_wmesg;			/* resource sleeping (for tsleep) */
    int					    rwl_timo;			/* maximum sleep time (for tsleep) */
};


#endif /* SYS_RWLOCK_H_ */
