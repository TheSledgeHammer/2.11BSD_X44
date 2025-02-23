/*	$NetBSD: pthread_debug.c,v 1.8 2004/03/14 01:19:41 cl Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_debug.c,v 1.8 2004/03/14 01:19:41 cl Exp $");

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "pthread.h"
#include "pthread_int.h"

int pthread__dbg;

#ifdef PTHREAD__DEBUG

int pthread__debug_counters[PTHREADD_NCOUNTERS];

#define	MAXLINELEN 1000
struct linebuf {
	char buf[MAXLINELEN];
	int len;
};
static struct linebuf *linebuf;

static void pthread__debug_printcounters(void);
static const char *pthread__counternames[] = PTHREADD_INITCOUNTERNAMES;

void
pthread__debug_init(int ncpu)
{
	time_t t;
	int i;

	if (getenv("PTHREAD_DEBUGCOUNTERS") != NULL) {
		atexit(pthread__debug_printcounters);
	}
	linebuf = (struct linebuf*) malloc(ncpu * sizeof(struct linebuf));
	if (linebuf == NULL) {
		err(1, "Couldn't allocate linebuf");
	}
	for (i = 0; i < ncpu; i++) {
		linebuf[i].len = 0;
	}
}

static void
pthread__debug_printcounters(void)
{
	int i;
	int counts[PTHREADD_NCOUNTERS];

	/*
	 * Copy the counters before printing anything to so that we don't see
	 * the effect of printing the counters.
	 * There will still be one more mutex lock than unlock, because
	 * atexit() handling itself will call us back with a mutex locked.
	 */
	for (i = 0; i < PTHREADD_NCOUNTERS; i++)
		counts[i] = pthread__debug_counters[i];

	printf("Pthread event counters\n");
	for (i = 0; i < PTHREADD_NCOUNTERS; i++)
		printf("%2d %-20s %9d\n", i, pthread__counternames[i], counts[i]);
	printf("\n");
}

void
pthread__debuglog_printf(const char *fmt, ...)
{
	char *tmpbuf;
	size_t len;
	int vpid;
	va_list ap;

	if (pthread__maxconcurrency > 1) {
		vpid = pthread_self()->pt_vpid;
	} else {
		vpid = 0;
	}

	tmpbuf = linebuf[vpid].buf;
	len = linebuf[vpid].len;

	va_start(ap, fmt);
	len += vsnprintf(tmpbuf + len, (unsigned int) (MAXLINELEN - len), fmt, ap);
	va_end(ap);

	linebuf[vpid].len = len;

	if (tmpbuf[len - 1] != '\n') {
		return;
	}

	linebuf[vpid].len = 0;
}

#endif
