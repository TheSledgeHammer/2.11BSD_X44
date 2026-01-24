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
 *     1.0  1997/9/25	1. Initial release.
*/

#include <sys/cdefs.h>

#include "namespace.h"

#ifdef __SELECT_DECLARED
#include <select.h>
#endif

#include <errno.h>
#include <stdio.h>	        /* For NULL */
#include <time.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(sleep,_sleep)
#endif

/*
 * This implements the sleep(3) function using only 3 system calls instead of 
 * the 9 that the old implementation required.  Also this version avoids using
 * signals (with the attendant system overhead) and returns the amount of 
 * time left unslept if an interrupt occurs.
 *
 * The error status of gettimeofday is not checked because if that fails the
 * program has scrambled the stack so badly that a sleep() failure is the least
 * problem the program has.  The select() call either completes successfully
 * or is interrupted - no errors to be checked for.
*/
#ifdef __SELECT_DECLARED
u_int
sleep(seconds)
	u_int seconds;
{
	struct timeval f, s;

	if (seconds) {
		gettimeofday(&f, NULL);
		s.tv_sec = seconds;
		s.tv_usec = 0;
		select(0, NULL, NULL, NULL, &s);
		gettimeofday(&s, NULL);
		seconds -= (s.tv_sec - f.tv_sec);
		/*
		 * ONLY way this can happen is if the system time gets set back while we're
		 * in the select() call.  In this case return 0 instead of a bogus number.
		 */
		if (seconds < 0)
			seconds = 0;
	}
	return (seconds);
}

#else /* !__SELECT_DECLARED */

u_int
sleep(seconds)
    u_int seconds;
{
    struct timespec f, s;

    if (seconds) {
        s.tv_sec = seconds;
		s.tv_nsec = 0;
        nanosleep(&f, &s);
        seconds -= (s.tv_sec - f.tv_sec);
		/*
		 * ONLY way this can happen is if the system time gets set back while we're
		 * in the nanosleep() call.  In this case return 0 instead of a bogus number.
		 */
        if (seconds < 0) {
            seconds = 0;
        }
    }
   	return (seconds);
}

#endif /* !__SELECT_DECLARED */
