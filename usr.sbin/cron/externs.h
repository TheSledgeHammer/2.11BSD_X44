/* Copyright 1993,1994 by Paul Vixie
 * All rights reserved
 *
 * Distribute freely, except: don't remove my name from the source or
 * documentation (don't take credit for my work), mark your changes (don't
 * get me blamed for your possible bugs), don't alter or remove this
 * notice.  May be sold if buildable source is provided to buyer.  No
 * warrantee of any kind, express or implied, is included with this
 * software; use at your own risk, responsibility for damages (if any) to
 * anyone resulting from the use of this software rests entirely with the
 * user.
 *
 * Send bug reports, bug fixes, enhancements, requests, flames, etc., and
 * I'll try to keep a version up to date.  I can be reached as follows:
 * Paul Vixie          <paul@vix.com>          uunet!decwrl!vixie!paul
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#if defined(POSIX) || defined(ATT)
#include <dirent.h>
#define DIR_T struct dirent
#define WAIT_IS_INT 1
#endif

#if !defined(POSIX) && !defined(ATT)
/* classic BSD */
#include <sys/dir.h>
#define DIR_T struct direct
#endif

#if defined(UNIXPC)
#undef WAIT_T
#undef WAIT_IS_INT
#endif

#if WAIT_IS_INT

#ifndef WEXITSTATUS
#define WEXITSTATUS(x)  (((x) >> 8) & 0xff)
#endif

#ifndef WTERMSIG
#define WTERMSIG(x)	    ((x) & 0x7f)
#endif

#ifndef WCOREDUMP
#define WCOREDUMP(x)	((x) & 0x80)
#endif

#else /* WAIT_IS_INT */

#ifndef WEXITSTATUS
#define	WEXITSTATUS(x)	((x).w_retcode)
#endif

#ifndef WTERMSIG
#define	WTERMSIG(x)		((x).w_termsig)
#endif

#ifndef WCOREDUMP
#define	WCOREDUMP(x)	((x).w_coredump)
#endif

#endif /* WAIT_IS_INT */

#ifdef NEED_SETSID
extern int	setsid(void);
#endif
