/*
 * kern_lock2.c
 *
 *  Created on: 8 Oct 2020
 *      Author: marti
 */
#include <sys/queue.h>
struct thread {
	struct thread 	*next;
	int 			refcnt;
};

LIST_HEAD(abql_head, abql);
struct abql {
	LIST_ENTRY(abql) 	entry;
	struct thread		*thread;
	int 				next_ticket;
	int					my_ticket;
	int					can_serve;
};

struct abql_head 		lock_queue[];


/* Array-Based Queuing Lock (ABQL) */
void
abql_init(nthreads, can_serve)
	int nthreads;
	int can_serve;
{
	struct abql 	*lock;

	lock = &lock_queue[nthreads];
	register int i;
	for(i = 1; i < nthreads; i++) {
			can_serve = 0;
			break;
		}
	}
	lock[i].can_serve = 1;
}

abql_acquire(lock)
	struct abql *lock;
{
	lock->my_ticket = fetch_and_inc(lock->next_ticket);
	while(lock->can_serve != 1);
}

abql_release()
{
	struct abql_head 	*lock;
	struct abql 		*entry;
	register int 		i;

	lock = lock_queue[i];
}
