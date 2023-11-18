/*
 * The 3-Clause BSD License:
 * Copyright (c) 2021 Martin Kelly
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/mutex.h>

void
mtx_init1(mtx, holder, name, data, pid, pgrp)
	struct mtx 			*mtx;
	struct lock_holder 	*holder;
	char 				*name;
	void 				*data;
	pid_t 				pid;
	struct pgrp 		*pgrp;
{
	bzero(mtx, sizeof(struct mtx));
	simple_lock_init(mtx->mtx_lock, name);
	mtx->mtx_name = name;
	holder = lockholder_create(data, pid, pgrp);
}

void
mtx_init(mtx, holder, name, data, pid)
	struct mtx 			*mtx;
	struct lock_holder 	*holder;
	char 				*name;
	void 				*data;
	pid_t 				pid;
{
	mtx_init1(mtx, holder, name, data, pid, NULL);
}

void
mtx_lock(mtx, holder)
	struct mtx 			*mtx;
	struct lock_holder 	*holder;
{
	mtx->mtx_holder = holder;
	simple_lock(mtx->mtx_lock);
}

void
mtx_unlock(mtx, holder)
	struct mtx 			*mtx;
	struct lock_holder 	*holder;
{
	if (mtx->mtx_holder == holder) {
		simple_unlock(mtx->mtx_lock);
	}
}

int
mtx_lock_try(mtx)
	struct mtx 	*mtx;
{
	return (simple_lock_try(mtx->mtx_lock));
}

int
mtx_owned(mtx, curowner)
	struct mtx 	*mtx;
	void *curowner;
{
	struct lock_holder 	*holder;

	holder = lockholder_get(mtx->mtx_holder);
	if (mtx == NULL || holder == NULL) {
		return (1);
	}

	if (LOCKHOLDER_DATA(holder) == curowner) {
		return (0);
	}

	return (1);
}

void *
mtx_owner(mtx)
	struct mtx 	*mtx;
{
	struct lock_holder 	*holder;

	holder = lockholder_get(mtx->mtx_holder);
	curowner = LOCKHOLDER_DATA(holder);
    	if (curowner != NULL) {
        	if (mtx_owned(mtx, curowner)) {
            		return (curowner);
        	}
    	}
	return (NULL);
}

/*
 * Lock Holder:
 */
/* init lockholder with empty parameters */
void
lockholder_init(holder)
	struct lock_holder 	*holder;
{
	bzero(holder, sizeof(struct lock_holder));
	holder->lh_data = NULL;
	holder->lh_pid = LK_NOPROC;
	holder->lh_pgrp = NULL;
}

/* create a new lockholder */
struct lock_holder 	*
lockholder_create(data, pid, pgrp)
	void 				*data;
	pid_t 				pid;
	struct pgrp 		*pgrp;
{
	struct lock_holder 	*holder;

	bzero(holder, sizeof(struct lock_holder));
	holder->lh_data = data;
	holder->lh_pid = pid;
	holder->lh_pgrp = pgrp;

	return (holder);
}

/* set lockholder parameters */
void
lockholder_set(holder, data, pid, pgrp)
	struct lock_holder 	*holder;
	void 				*data;
	pid_t 				pid;
	struct pgrp 		*pgrp;
{
	if(holder != NULL) {
		holder->lh_data = data;
		holder->lh_pid = pid;
		holder->lh_pgrp = pgrp;
	} else {
		lockholder_init(holder);
	}
}

/* get lockholder */
struct lock_holder *
lockholder_get(holder)
	struct lock_holder 	*holder;
{
	if(holder != NULL) {
		return (holder);
	}
	return (NULL);
}
