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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <rwlock.h>
#include <kthread.h>
#include <uthread.h>

/* Initialize a rwlock */
void
rwlock_init(rwl, prio, wmesg, timo, flags)
	rwlock_t rwl;
	char *wmesg;
	int prio, timo;
	unsigned int flags;
{
	rwl->rwl_lock = 0;
	rwl->rwl_flags = flags & RW_EXTFLG_MASK;
	rwl->rwl_prio = prio;
	rwl->rwl_timo = timo;
	rwl->rwl_wmesg = wmesg;
	rwl->rwl_lockholder = RW_NOTHREAD;
    rwl->rwl_ktlockholder = NULL;
    rwl->rwl_utlockholder = NULL;
    simple_lock_init(&rwl->rwl_interlock);
}

/* Initialize a reader-writers lock via mutex */
void
rwlock_mutex_init2(rwl, mtx)
	rwlock_t rwl;
	mutex_t mtx;
{
	if(mtx == NULL) {
		mutex_init(mtx, mtx->mtx_prio, mtx->mtx_wmesg, mtx->mtx_timo, mtx->mtx_flags);
	}
	rwl->rwl_mutex = mtx;
	rwl->rwl_lock = mtx->mtx_lock;
	rwl->rwl_flags = mtx->mtx_flags;
	rwl->rwl_prio = mtx->mtx_prio;
	rwl->rwl_timo = mtx->mtx_timo;
	rwl->rwl_wmesg = mtx->mtx_wmesg;
	rwl->rwl_lockholder = mtx->mtx_lockholder;
    rwl->rwl_ktlockholder = mtx->mtx_ktlockholder;
    rwl->rwl_utlockholder = mtx->mtx_utlockholder;
}

/* Not yet implemented */
int
rwlock_destroy(rwlock_t)
{
	return (0);
}

/* Not yet implemented */
int
rwlock_tryenter(rwlock_t, const rwl_t)
{
	return (0);
}

/* Not yet implemented */
int
rwlock_tryupgrade(rwlock_t)
{
	return (0);
}

/* Not yet implemented */
void
rwlock_downgrade(rwlock_t)
{

}

/* Not yet implemented */
int
rwlock_read_held(rwlock_t)
{
	return (0);
}

/* Not yet implemented */
int
rwlock_write_held(rwlock_t)
{
	return (0);
}

/* Not yet implemented */
int
rwlock_lock_held(rwlock_t)
{
	return (0);
}

/* Not yet implemented */
void
rwlock_enter(rwlock_t, const rwl_t)
{

}

/* Not yet implemented */
void
rwlock_exit(rwlock_t)
{

}
