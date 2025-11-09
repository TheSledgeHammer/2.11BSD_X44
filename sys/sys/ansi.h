/*	$NetBSD: ansi.h,v 1.11 2005/12/11 12:25:20 christos Exp $	*/

/*-
 * Copyright (c) 2000, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jun-ichiro itojun Hagino and by Klaus Klein.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef	_SYS_ANSI_H_
#define	_SYS_ANSI_H_

#include <sys/cdefs.h>
//#include <machine/ansi.h>
#include <machine/types.h>

typedef unsigned long			__fixpt_t;	/* fixed point number */
typedef	unsigned short			__nlink_t;	/* link count */
//typedef	long				__segsz_t;	/* segment size */
typedef long				__daddr_t;	/* disk address */
typedef char 				*__caddr_t;	/* core address */
typedef unsigned long			__ino_t;	/* inode number*/
typedef	long				__swblk_t;	/* swap offset */
//typedef	long				__time_t;	/* time? */
typedef unsigned long			__dev_t;	/* device number */
typedef long				__memaddr_t;	/* core & swap address */
typedef __uint32_t			__gid_t;	/* group id */
typedef __uint32_t			__in_addr_t;	/* IP(v4) address */
typedef __uint16_t			__in_port_t;	/* "Internet" port number */
typedef __uint32_t			__mode_t;	/* file permissions */
typedef __int64_t			__off_t;	/* file offset */
typedef __int32_t			__pid_t;	/* process id */
typedef __int32_t 			__pri_t;	/* process priority */
typedef __uint8_t			__sa_family_t;	/* socket address family */
typedef int				__socklen_t;	/* socket-related datum length */
typedef __uint32_t			__uid_t;	/* user id */
typedef	__uint64_t			__fsblkcnt_t;	/* fs block count (statvfs) */
typedef	__uint64_t			__fsfilcnt_t;	/* fs file count */

struct __tag_wctrans_t;

/*
 * Types which are fundamental to the implementation and may appear in
 * more than one standard header are defined here.  Standard headers
 * then use:
 *	#ifdef	_BSD_SIZE_T_
 *	typedef	_BSD_SIZE_T_ size_t;
 *	#undef	_BSD_SIZE_T_
 *	#endif
 */
#define	_BSD_CLOCK_T_		unsigned long	/* clock() */
#define	_BSD_CLOCKID_T_		int				/* clockid_t */
#define	_BSD_PTRDIFF_T_		int				/* ptr1 - ptr2 */
#define	_BSD_SIZE_T_		unsigned int	/* sizeof() */
#define	_BSD_SSIZE_T_		int				/* byte count or error */
#define	_BSD_TIME_T_		__int64_t		/* time() */
#define	_BSD_VA_LIST_		char *			/* va_list */
#define	_BSD_TIMER_T_		int				/* timer_t */
#define	_BSD_SUSECONDS_T_	int				/* suseconds_t */
#define	_BSD_USECONDS_T_	unsigned int	/* useconds_t */
#define	_BSD_INTPTR_T_		int				/* intptr_t */
#define	_BSD_UINTPTR_T_		unsigned int	/* uintptr_t */

/*
 * Runes (wchar_t) is declared to be an ``int'' instead of the more natural
 * ``unsigned long'' or ``long''.  Two things are happening here.  It is not
 * unsigned so that EOF (-1) can be naturally assigned to it and used.  Also,
 * it looks like 10646 will be a 31 bit standard.  This means that if your
 * ints cannot hold 32 bits, you will be in trouble.  The reason an int was
 * chosen over a long is that the is*() and to*() routines take ints (says
 * ANSI C), but they use _RUNE_T_ instead of int.  By changing it here, you
 * lose a bit of ANSI conformance, but your programs will still work.
 *
 * Note that _WCHAR_T_ and _RUNE_T_ must be of the same type.  When wchar_t
 * and rune_t are typedef'd, _WCHAR_T_ will be undef'd, but _RUNE_T remains
 * defined for ctype.h.
 */

/*
 * mbstate_t is an opaque object to keep conversion state, during multibyte
 * stream conversions.  The content must not be referenced by user programs.
 */
typedef union {
	__int64_t 	__mbstateL; 				/* for alignment */
	char 		__mbstate8[128];
} __mbstate_t;

#define	_BSD_WCHAR_T_		__wchar_t		/* wchar_t */
#define _BSD_WINT_T_		__wint_t		/* wint_t */
#define	_BSD_RUNE_T_		__rune_t		/* rune_t */

#define _BSD_WCTRANS_T_		__wctrans_t		/* wctrans_t */
#define _BSD_WCTYPE_T_		__wctype_t		/* wctype_t */
#define _BSD_MBSTATE_T_		__mbstate_t		/* mbstate_t */

#ifdef __lint__
typedef char *__va_list;
#else
typedef __builtin_va_list __va_list;
#endif

#endif	/* !_SYS_ANSI_H_ */
