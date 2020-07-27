/*
 * Copyright (c) 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code contains ideas from software contributed to Berkeley by
 * Avadis Tevanian, Jr., Michael Wayne Young, and the Mach Operating
 * System project at Carnegie-Mellon University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Reworked from: @(#)lock.h	8.12 (Berkeley) 5/19/95
 */

#ifndef SYS_MUTEX_H_
#define SYS_MUTEX_H_

struct mutex {
	volatile u_int   		mtx_lock;

	struct proc				*mtx_prlockholder;	/* proc lock holder */
    struct kthread          *mtx_ktlockholder; 	/* kernel thread lock holder */
    struct uthread          *mtx_utlockholder;	/* user thread lock holder */

    pid_t                   mtx_lockholder_pid;

    struct simplelock 	    *mtx_interlock; 	/* lock on remaining fields */

    struct lock_object		*mtx_lockobject;	/* lock object (to replace simplelock) */
};

typedef struct mutex        *mutex_t;

/* Generic Mutex Functions */
void mutex_lock(__volatile mutex_t);
void mutex_unlock(__volatile mutex_t);
int mutex_enter(__volatile mutex_t);
int mutex_exit(__volatile mutex_t);
int mutex_lock_try(__volatile mutex_t);

#endif /* SYS_MUTEX_H_ */
