/*
 * user_thread.c
 *
 *  Created on: 2 May 2020
 *      Author: marti
 */
#include<sys/malloc.h>
#include<uthread.h>

int
uthread_create(uthread_t ut)
{
	ut->ut_flag = TSWAIT | TSREADY;
}

int
uthread_join(uthread_t ut)
{

}

int
uthread_cancel(uthread_t ut)
{

}

int
uthread_exit(uthread_t ut)
{

}

int
uthread_detach(uthread_t ut)
{

}

int
uthread_equal(uthread_t ut1, uthread_t ut2)
{
	if(ut1 > ut2) {
		return (1);
	} else if(ut1 < ut2) {
		return (-1);
	}
	return (0);
}

int
uthread_kill(uthread_t ut)
{

}

