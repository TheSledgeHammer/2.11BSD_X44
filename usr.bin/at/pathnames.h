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

#ifndef _PATHNAMES_H_
#define	_PATHNAMES_H_

#include <paths.h>

#define BOURNE			_PATH_BSHELL	/* run commands with Bourne shell*/
#define CSHELL			_PATH_CSHELL	/* run commands with C shell */

#define TMPDIR			_PATH_TMP					/* area for temporary files */
#define MAILER			"/bin/mail"					/* program to use for sending mail */

#ifdef OLD_PATHS
#define ATDIR_OLD      "/usr/spool/at"
#define PASTDIR_OLD    "/usr/spool/at/past"
#define LASTFILE_OLD   "/usr/spool/at/lasttimedone"
#define ATDIR			ATDIR_OLD					/* spooling area */
#define PASTDIR			PASTDIR_OLD
#define LASTFILE		LASTFILE_OLD				/* update time record file */
#else
#define ATDIR_NEW      "/usr/var/at/spool"
#define PASTDIR_NEW    "/usr/var/at/spool/past"
#define LASTFILE_NEW   "/usr/var/at/spool/lasttimedone"
#define ATDIR			ATDIR_NEW					/* spooling area */
#define PASTDIR			PASTDIR_NEW
#define LASTFILE		LASTFILE_NEW				/* update time record file */
#endif

#endif /* _PATHNAMES_H_ */
