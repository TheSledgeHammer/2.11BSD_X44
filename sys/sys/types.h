/*-
 * Copyright (c) 1982, 1986, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)types.h	8.6 (Berkeley) 2/19/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)types.h	1.4.1 (2.11BSD) 2000/2/28
 */

#ifndef _SYS_TYPES_H
#define	_SYS_TYPES_H

/* Machine type dependent parameters. */
#include <machine/ansi.h>
#include <machine/types.h>

#include <sys/ansi.h>

#ifndef	_BSD_INT8_T_
typedef	__int8_t			int8_t;
#define	_BSD_INT8_T_
#endif

#ifndef	_BSD_UINT8_T_
typedef	__uint8_t			uint8_t;
#define	_BSD_UINT8_T_
#endif

#ifndef	_BSD_INT16_T_
typedef	__int16_t			int16_t;
#define	_BSD_INT16_T_
#endif

#ifndef	_BSD_UINT16_T_
typedef	__uint16_t			uint16_t;
#define	_BSD_UINT16_T_
#endif

#ifndef	_BSD_INT32_T_
typedef	__int32_t			int32_t;
#define	_BSD_INT32_T_
#endif

#ifndef	_BSD_UINT32_T_
typedef	__uint32_t			uint32_t;
#define	_BSD_UINT32_T_
#endif

#ifndef	_BSD_INT64_T_
typedef	__int64_t			int64_t;
#define	_BSD_INT64_T_
#endif

#ifndef	_BSD_UINT64_T_
typedef	__uint64_t			uint64_t;
#define	_BSD_UINT64_T_
#endif

#ifndef	_BSD_INTPTR_T_
typedef	__intptr_t			intptr_t;
#define	_BSD_INTPTR_T_
#endif

#ifndef	_BSD_UINTPTR_T_
typedef	__uintptr_t			uintptr_t;
#define	_BSD_UINTPTR_T_
#endif

#ifndef _BSD_REGISTER_T_
typedef	__register_t 		register_t;
#define _BSD_REGISTER_T_
#endif

#ifndef _BSD_UREGISTER_T_
typedef	__uregister_t 		uregister_t;
#define _BSD_UREGISTER_T_
#endif

/* BSD-style unsigned bits types */
typedef	uint8_t				u_int8_t;
typedef	uint16_t			u_int16_t;
typedef	uint32_t			u_int32_t;
typedef	uint64_t			u_int64_t;

#ifndef _POSIX_SOURCE
typedef	unsigned char		u_char;
typedef	unsigned short		u_short;
typedef	unsigned int		u_int;
typedef	unsigned long		u_long;

typedef unsigned char		unchar;		/* Sys III/V compatibility */
typedef	unsigned short		ushort;		/* Sys III/V compatibility */
typedef	unsigned int		uint;		/* Sys III/V compatibility */
typedef unsigned long		ulong;		/* Sys III/V compatibility */
#endif /* _POSIX_SOURCE */

typedef	uint64_t	 		u_quad_t; 	/* quads */
typedef	int64_t				quad_t;
typedef	quad_t 				*qaddr_t;

#if defined(_KERNEL)
typedef u_long				fixpt_t;	/* fixed point number */
typedef	u_short				nlink_t;	/* link count */
typedef	long				segsz_t;	/* segment size */
typedef	long				daddr_t;	/* disk address */
typedef	char 				*caddr_t;	/* core address */
typedef	u_long				ino_t;		/* inode number*/
typedef	long				swblk_t;	/* swap offset */
//typedef	long				time_t;		/* time? */
typedef	u_long				dev_t;		/* device number */
typedef	quad_t				off_t;		/* file offset */
typedef	u_short				mode_t;		/* permissions */
typedef long				memaddr_t;	/* core & swap address */
typedef u_quad_t			fsblkcnt_t; /* fs block count (statvfs) */
typedef u_quad_t			fsfilcnt_t; /* fs file count */

typedef	u_long				gid_t;		/* group id */
typedef	u_char	    		pid_t;		/* process id */
typedef u_char 				pri_t;		/* process priority */
typedef	u_long				uid_t;		/* user id */

typedef uint32_t			in_addr_t;	/* IP(v4) address */
typedef uint16_t			in_port_t;	/* "Internet" port number */
#else
typedef __fixpt_t 			fixpt_t;	/* fixed point number */
typedef	__nlink_t 			nlink_t;	/* link count */
typedef	long     			segsz_t;	/* segment size */
typedef uint64_t			rlim_t;		/* resource limit */
typedef __daddr_t			daddr_t;	/* disk address */
typedef __caddr_t 			caddr_t;	/* core address */
typedef __ino_t				ino_t;		/* inode number*/
typedef	__swblk_t 			swblk_t;	/* swap offset */
//typedef	long 			    time_t;		/* time? */
typedef __dev_t				dev_t;		/* device number */
typedef __off_t 			off_t;		/* file offset */
typedef __mode_t 			mode_t;		/* permissions */
typedef __memaddr_t			memaddr_t;	/* core & swap address */
typedef __fsblkcnt_t		fsblkcnt_t; /* fs block count (statvfs) */
typedef __fsfilcnt_t		fsfilcnt_t; /* fs file count */
typedef __gid_t 			gid_t;		/* group id */
typedef __pid_t 			pid_t;		/* process id */
typedef	__pri_t				pri_t;		/* process priority */
typedef __uid_t 			uid_t;		/* user id */
typedef __in_addr_t			in_addr_t;	/* IP(v4) address */
typedef __in_port_t			in_port_t;	/* "Internet" port number */
typedef	uint32_t	        id_t;		/* group id, process id or user id */
#endif /* _KERNEL */

#if defined(_KERNEL) || defined(_LIBC)
#include <sys/stdint.h>
typedef intptr_t 			semid_t;
#endif /* _KERNEL || _LIBC */

#include <sys/select.h>

/*
 * Basic system types and major/minor device constructing/busting macros.
 */
#define	major(x)			((int)(((int)(x)>>8)&0377))		/* major part of a device */
#define	minor(x)			((int)((x)&0377)) 				/* minor part of a device */
#define	makedev(x,y)		((dev_t)(((x)<<8)|(y)))			/* make a device number */

#define	NBBY				8								/* number of bits in a byte */

#ifndef howmany
#define	howmany(x, y)		(((x)+((y)-1))/(y))
#endif

#include <machine/endian.h>

#ifdef	_BSD_CLOCK_T_
typedef	_BSD_CLOCK_T_		clock_t;
#undef	_BSD_CLOCK_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_		size_t;
#undef	_BSD_SIZE_T_
#endif

#ifdef	_BSD_SSIZE_T_
typedef	_BSD_SSIZE_T_		ssize_t;
#undef	_BSD_SSIZE_T_
#endif

#ifdef	_BSD_TIME_T_
typedef	_BSD_TIME_T_		time_t;
#undef	_BSD_TIME_T_
#endif

#ifdef	_BSD_CLOCKID_T_
typedef	_BSD_CLOCKID_T_		clockid_t;
#undef	_BSD_CLOCKID_T_
#endif

#ifdef	_BSD_TIMER_T_
typedef	_BSD_TIMER_T_		timer_t;
#undef	_BSD_TIMER_T_
#endif

#if defined(_KERNEL) || defined(_STANDALONE)
#define SET(t, f)			((t) |= (f))
#define	ISSET(t, f)			((t) & (f))
#define	CLR(t, f)			((t) &= ~(f))
#endif

#if defined(__STDC__) && defined(_KERNEL)
/*
 * Forward structure declarations for function prototypes.  We include the
 * common structures that cross subsystem boundaries here; others are mostly
 * used in the same place that the structure is defined.
 */
struct	proc;
struct	pgrp;
struct	ucred;
struct	rusage;
struct	k_rusage;
struct	file;
struct	buf;
struct	tty;
struct	uio;
struct	user;
#endif

#if defined(_KERNEL) || defined(_STANDALONE)

#include <sys/stdbool.h>
/* 2.11BSD bool_t type. Needed for backwards compatability. */
typedef char	bool_t;

/*
 * Deprecated Mach-style boolean_t type.  Should not be used by new code.
 */
typedef int		boolean_t;
#ifndef TRUE
#define	TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif
#endif /* _KERNEL || _STANDALONE */
#endif
