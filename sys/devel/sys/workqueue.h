/*
 * workqueue.h
 *
 *  Created on: 18 Sep 2020
 *      Author: marti
 *
 *  Derived From:  https://github.com/skeeto/lqueue
 */


#ifndef _SYS_WORKQUEUE_H_
#define _SYS_WORKQUEUE_H_

#include <sys/cdefs.h>
#include <devel/sys/stdatomic.h>
#include <semaphore.h>

typedef void 		*sem_t;

struct lqueue {
	atomic_ulong  	head;
    atomic_ulong  	tail;
    unsigned long 	mask;
    size_t 			element_size;
    char 			buffer[];
};
typedef struct lqueue lqueue;

struct wqueue {
	lqueue 				*lqueue;
	sem_t 				count;
	int 				nthreads;
	struct wqueue_data {
		kthread_t 		thread;
		int 			id;
		struct wqueue 	*q;
		sem_t 			pause;
		sem_t 			complete;
	} threads[];
};
typedef struct wqueue wqueue;

struct job {
    void (*f)(int, void *);
    void *arg;
};

lqueue *lqueue_create(unsigned min_size, size_t element_size);
void    lqueue_free(lqueue *);
int     lqueue_offer(lqueue *, void *);
int     lqueue_poll(lqueue *, void *);

wqueue *wqueue_create(unsigned min_size, int nthreads);
void    wqueue_free(wqueue *);
void    wqueue_add(wqueue *, void (*)(int, void *), void *);
void    wqueue_wait(wqueue *);

#endif /* _SYS_WORKQUEUE_H_ */
