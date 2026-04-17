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

/*
 * $Id: compat.h,v 1.8 1994/01/15 20:43:43 vixie Exp $
 */

#ifndef __P
#define __P(x) ()
#define const
#endif

#if defined(UNIXPC) || defined(unixpc)
#define UNIXPC 1
#define ATT 1
#endif

#ifndef POSIX
#if (BSD >= 199103) || defined(__linux) || defined(ultrix) || defined(AIX) ||\
	defined(HPUX) || defined(CONVEX) || defined(IRIX)
#define POSIX
#endif
#endif

#ifndef BSD
#if defined(ultrix)
#define BSD 198902
#endif
#endif

/*****************************************************************/

#if defined(POSIX) || defined(ATT)
#define WAIT_T	int
#endif

#if !defined(POSIX) && !defined(ATT)
/* classic BSD */
#define	WAIT_T	union wait
#endif

#define	PID_T	pid_t
#define	TIME_T	time_t
