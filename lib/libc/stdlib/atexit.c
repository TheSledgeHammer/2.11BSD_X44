/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)atexit.c	8.2 (Berkeley) 7/3/94";
#endif
#endif /* LIBC_SCCS and not lint */

#include "reentrant.h"

#include <stddef.h>
#include <stdlib.h>

#include "atexit.h"

struct atexit *__atexit;	/* points to head of LIFO stack */

#ifdef _REENTRANT
/* ..and a mutex to protect it all. */
mutex_t __atexit_mutex;
#endif /* _REENTRANT */

void atexit_init(void) __attribute__ ((visibility("hidden")));

static int atexit_alloc(struct atexit_fun *);
static int common_atexit(void (*)(void), void (*)(void *), void *, void *);

/*
 * Register a function to be performed at exit.
 */
static int
atexit_alloc(fun)
	struct atexit_fun *fun;
{
	static struct atexit __atexit0;	/* one guaranteed table */
	register struct atexit *p;

	mutex_lock(&__atexit_mutex);
	if ((p = __atexit) == NULL) {
		__atexit = p = &__atexit0;
	} else if (p->ind >= ATEXIT_SIZE) {
		if ((p = malloc(sizeof(*p))) == NULL) {
			mutex_unlock(&__atexit_mutex);
			return (-1);
		}
		p->ind = 0;
		p->next = __atexit;
		__atexit = p;
	}
	p->fns[p->ind++] = *fun;
	mutex_unlock(&__atexit_mutex);
	return (0);
}

static int
common_atexit(func, cxa_func, arg, dso)
	void (*func)(void);
	void (*cxa_func)(void *);
	void *arg, *dso;
{
	struct atexit_fun fun;
	int error;

	mutex_lock(&__atexit_mutex);
	fun.fun_atexit = func;
	fun.fun_cxa_atexit = cxa_func;
	fun.fun_arg = arg;
	fun.fun_dso = dso;
	error = atexit_alloc(&fun);
	mutex_unlock(&__atexit_mutex);
	return (error);
}

int
atexit(func)
	void (*func)(void);
{
	return (common_atexit(func, NULL, NULL, NULL));
}

int
__cxa_atexit(cxa_func, arg, dso)
	void (*cxa_func)(void *);
	void *arg, *dso;
{
	return (common_atexit(NULL, cxa_func, arg, dso));
}

void
atexit_init(void)
{
#ifdef _REENTRANT
	mutexattr_t atexit_mutex_attr;
	mutexattr_init(&atexit_mutex_attr);
	mutexattr_settype(&atexit_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    mutex_init(&__atexit_mutex, &atexit_mutex_attr);
#endif
}

void
__cxa_finalize(dso)
	void *dso;
{
	struct atexit *p;
	struct atexit_fun fun;
	int n;

	mutex_lock(&__atexit_mutex);
	for (p = __atexit; p; p = p->next) {
		for (n = p->ind; --n >= 0;) {
			fun = p->fns[n];
			if ((dso != NULL) && (fun.fun_dso != NULL)) {
				if (dso != fun.fun_dso) {
					continue;
				}
			}
			if (fun.fun_cxa_atexit != NULL) {
				fun.fun_cxa_atexit(fun.fun_arg);
			}
			if (fun.fun_atexit != NULL) {
				fun.fun_atexit();
			}
		}
	}
	mutex_unlock(&__atexit_mutex);
}
