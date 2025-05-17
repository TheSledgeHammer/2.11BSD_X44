/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
static char sccsid[] = "@(#)exit.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>
#include <unistd.h>

#include "reentrant.h"
#include "atexit.h"

extern void (*__cleanup)(void);
//void (*__cleanup)();

/*
 * Exit, flushing stdio buffers if necessary.
 */
void
exit(code)
	int code;
{
/*
	register struct atexit *p;
	register int n;

	for (p = __atexit; p; p = p->next) {
		for (n = p->ind; --n >= 0;) {
			(*p->fns[n])();
		}
	}
*/
	__cxa_finalize(NULL);
	if (__cleanup) {
		(*__cleanup)();
	}
	_exit(code);
}

/*
 * _Exit() is the ISO/ANSI C99 equivalent of the POSIX _exit() function.
 * No atexit() handlers are called and no signal handlers are run.
 * Whether or not stdio buffers are flushed or temporary files are removed
 * is implementation-dependent in C99.  Indeed, POSIX specifies that
 * _Exit() must *not* flush stdio buffers or remove temporary files, but
 * rather must behave exactly like _exit()
 */
void
_Exit(code)
	int code;
{
	_exit(code);
}

/**
 * Linked list of quick exit handlers.  This is simpler than the atexit()
 * version, because it is not required to support C++ destructors or
 * DSO-specific cleanups.
 */
struct quick_exit_handler {
	struct quick_exit_handler *next;
	void (*cleanup)(void);
};

/**
 * Lock protecting the handlers list.
 */
#ifdef _REENTRANT
extern mutex_t __atexit_mutex;
#endif

/**
 * Stack of cleanup handlers.  These will be invoked in reverse order when
 */
static struct quick_exit_handler *handlers;

int
at_quick_exit(func)
	void (*func)(void);
{
	struct quick_exit_handler *h;

	h = malloc(sizeof(*h));

	if (NULL == h) {
		return (1);
	}
	h->cleanup = func;
#ifdef _REENTRANT
	mutex_lock(&__atexit_mutex);
#endif
	h->next = handlers;
	handlers = h;
#ifdef _REENTRANT
	mutex_unlock(&__atexit_mutex);
#endif
	return (0);
}

void
quick_exit(code)
	int code;
{
	struct quick_exit_handler *h;

	/*
	 * XXX: The C++ spec requires us to call std::terminate if there is an
	 * exception here.
	 */
	for (h = handlers; NULL != h; h = h->next) {
		(*h->cleanup)();
	}
	_Exit(code);
}
