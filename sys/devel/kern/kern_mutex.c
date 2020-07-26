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
#include "devel/sys/mutex.h"
#include "devel/sys/kthread.h"
#include "devel/sys/uthread.h"

void
mutex_lock(mtx)
    __volatile mutex_t mtx;
{
    if (mtx->mtx_lock == 1) {
    	simple_lock(&mtx->mtx_interlock);
    	//simple_lock(&mtx->mtx_lockobject);
    }
}

void
mutex_unlock(mtx)
    __volatile mutex_t mtx;
{
    if (mtx->mtx_lock == 0) {
    	simple_unlock(&mtx->mtx_interlock);
    	//simple_unlock(&mtx->mtx_lockobject);
    }
}

int
mutex_enter(mtx)
	__volatile mutex_t mtx;
{
	if(mtx->mtx_lock != 1) {
		mtx_lock(mtx);
	}
	return (0);
}

int
mutex_exit(mtx)
	__volatile mutex_t mtx;
{
	if(mtx->mtx_lock != 0) {
		mtx_unlock(mtx);
	}
	return (0);
}

int
mutex_lock_try(mtx)
    __volatile mutex_t mtx;
{
	//return (simple_lock_try(&mtx->mtx_lockobject));
    return (simple_lock_try(&mtx->mtx_interlock));
}
