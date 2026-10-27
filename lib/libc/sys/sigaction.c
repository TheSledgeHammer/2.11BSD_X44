/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

/*
 * Sigtramp:
 * - Most modern c compilers should take care of this.
 * - Hence it should only return 0. Unless under special situations/conditions.
 * Example Situations:
 * 1) You know what you are doing!
 * 2) The c compiler doesn't support it. 2.11BSD's PCC is such a case.
 * 3) Any others not mentioned above.
 */

/*
 * Sigaction: (kernel syscall)
 * NOTES: On implementing sigtramp, please follow below.
 * 1) Sigaction only acknowledges the sigtramp argument when the return value is not
 * equal to 0, else will be ignored.
 * 2) Place in the 'libc/arch/machine/sys' folder.
 * 3) Add 'CPPFLAGS+= -DUSE_SIGTRAMP' to the Makefile.
 */
#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/syscall.h>

#include <signal.h>
#include <unistd.h>

/* This should really be implemented in 'libc/arch/machine/sys/' in assembly. */
#if defined(USE_SIGTRAMP)
/* could also be header */
extern int sigtramp(void);
#endif

static int __sigtramp(void);

static int
__sigtramp(void)
{
	int ret;

#if !defined(USE_SIGTRAMP)
	ret = 0;
#else
	ret = sigtramp();
#endif
	return (ret);
}

/* ingores 1st argument; int sigtramp(void) (this is a temporary solution) */
int
sigaction(int signum, const struct sigaction *nsa, struct sigaction *osa)
{
	return (__syscall((quad_t)SYS_sigaction, __sigtramp(), signum, nsa, osa));
}
