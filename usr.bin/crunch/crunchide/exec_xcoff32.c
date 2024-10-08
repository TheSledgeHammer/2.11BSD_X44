/* $NetBSD: exec_ecoff.c,v 1.7 2009/04/18 11:30:30 tsutsui Exp $ */

/*
 * Copyright (c) 1997 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

#if (defined(NLIST_XCOFF32) && (XCOFFSIZE == 32)) || \
    (defined(NLIST_XCOFF64) && (XCOFFSIZE == 64))

#include <sys/exec_xcoff.h>

int
XCOFFNAMEEND(check)(int fd, const char *filename)
{
	xcoff_exechdr eh;
	struct stat sb;

	/*
	 * Check the header to make sure it's an ECOFF file (of the
	 * appropriate size).
	 */
	if (fstat(fd, &sb) == -1)
		return 0;
	if (sb.st_size < (off_t)sizeof eh)
		return 0;
	if (read(fd, &eh, sizeof eh) != sizeof eh)
		return 0;

	if (XCOFF_BADMAG(&eh))
		return 0;

	return 1;
}

int
XCOFFNAMEEND(hide)(int fd, const char *filename)
{

	fprintf(stderr, "%s: XCOFF executables not currently supported\n", filename);
	return 1;
}

#endif /* defined(NLIST_ECOFF) */
