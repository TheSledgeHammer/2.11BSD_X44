/*
 * Copyright (c) 1989, 1993
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
/*
 * Program: sleep.c
 * Copyright: 1997, sms
 * Author: Steven M. Schultz
 *
 * Version   Date	Modification
 *     1.0  1997/9/26	1. Initial release.
*/

#include <sys/cdefs.h>

#include "namespace.h"

#ifdef __SELECT_DECLARED
#include <select.h>
#endif

#include <errno.h>
#include <stdio.h>	/* For NULL */
#include <time.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(usleep,_usleep)
#endif

/*
 * This implements the usleep(3) function using only 1 system call (select)
 * instead of the 9 that the old implementation required.  Also this version 
 * avoids using signals (with the attendant system overhead).
 *
 * Nothing is returned and if less than ~20000 microseconds is specified the
 * select will return without any delay at all.
*/

#ifdef __SELECT_DECLARED

int
usleep(micros)
    long micros;
{
	struct timeval s;

	if (micros > 0) {
		s.tv_sec = (micros / 1000000L);
		s.tv_usec = (micros % 1000000L);
		select(0, NULL, NULL, NULL, &s);
	}
    return (0);
}

#else /* !__SELECT_DECLARED */

int
usleep(micros)
    long micros;
{
	struct timespec s;

	if (micros > 0) {
		s.tv_sec = (micros / 1000000L);
		s.tv_nsec = (micros % 1000000L) * 1000;
		nanosleep(&s, NULL);
	}
    return (0);
}

#endif /* !__SELECT_DECLARED */
