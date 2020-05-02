/*
 * kern_kthread.c
 *
 *  Created on: 2 May 2020
 *      Author: marti
 */
#include<sys/malloc.h>
#include<kthread.h>


int
kthread_create(kthread_t kt)
{

}

int
kthread_join(kthread_t kt)
{

}

int
kthread_cancel(kthread_t kt)
{

}

int
kthread_exit(kthread_t kt)
{

}

int
kthread_detach(kthread_t kt)
{

}

int
kthread_equal(kthread_t kt1, kthread_t kt2)
{
	if(kt1 > kt2) {
		return (1);
	} else if(kt1 < kt2) {
		return (-1);
	}
	return (0);
}

int
kthread_kill(kthread_t kt)
{

}

