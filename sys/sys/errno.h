/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)errno.h	8.5 (Berkeley) 1/21/94
 */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)errno.h	7.1.3 (2.11BSD) 1999/9/6
 */

#ifndef	_SYS_ERRNO_H_
#define	_SYS_ERRNO_H_

#ifndef	_KERNEL
extern int				errno;	/* global error number */
#endif

#define	EPERM			1		/* Not owner */
#define	ENOENT			2		/* No such file or directory */
#define	ESRCH			3		/* No such process */
#define	EINTR			4		/* Interrupted system call */
#define	EIO				5		/* I/O error */
#define	ENXIO			6		/* No such device or address */
#define	E2BIG			7		/* Arg list too long */
#define	ENOEXEC			8		/* Exec format error */
#define	EBADF			9		/* Bad file number */
#define	ECHILD			10		/* No children */

#define	EAGAIN			11		/* No more processes */

#define	ENOMEM			12		/* Not enough core */
#define	EACCES			13		/* Permission denied */
#define	EFAULT			14		/* Bad address */
#define	ENOTBLK			15		/* Block device required */
#define	EBUSY			16		/* Mount device busy */
#define	EEXIST			17		/* File exists */
#define	EXDEV			18		/* Cross-device link */
#define	ENODEV			19		/* No such device */
#define	ENOTDIR			20		/* Not a directory*/
#define	EISDIR			21		/* Is a directory */
#define	EINVAL			22		/* Invalid argument */
#define	ENFILE			23		/* File table overflow */
#define	EMFILE			24		/* Too many open files */
#define	ENOTTY			25		/* Not a typewriter */
#define	ETXTBSY			26		/* Text file busy */
#define	EFBIG			27		/* File too large */
#define	ENOSPC			28		/* No space left on device */
#define	ESPIPE			29		/* Illegal seek */
#define	EROFS			30		/* Read-only file system */
#define	EMLINK			31		/* Too many links */
#define	EPIPE			32		/* Broken pipe */
#define	EMPX			EPIPE	/* Broken mpx */

/* math software */
#define	EDOM			33		/* Argument too large */
#define	ERANGE			34		/* Result too large */

/* non-blocking and interrupt i/o */
#define	EWOULDBLOCK		35		/* Operation would block */
#define	EDEADLK			EWOULDBLOCK	/* ditto */
#define	EINPROGRESS		36		/* Operation now in progress */
#define	EALREADY		37		/* Operation already in progress */

/* ipc/network software */

/* argument errors */
#define	ENOTSOCK		38		/* Socket operation on non-socket */
#define	EDESTADDRREQ	39		/* Destination address required */
#define	EMSGSIZE		40		/* Message too long */
#define	EPROTOTYPE		41		/* Protocol wrong type for socket */
#define	ENOPROTOOPT		42		/* Protocol not available */
#define	EPROTONOSUPPORT	43		/* Protocol not supported */
#define	ESOCKTNOSUPPORT	44		/* Socket type not supported */
#define	EOPNOTSUPP		45		/* Operation not supported on socket */
#define	EPFNOSUPPORT	46		/* Protocol family not supported */
#define	EAFNOSUPPORT	47		/* Address family not supported by protocol family */
#define	EADDRINUSE		48		/* Address already in use */
#define	EADDRNOTAVAIL	49		/* Can't assign requested address */

/* operational errors */
#define	ENETDOWN		50		/* Network is down */
#define	ENETUNREACH		51		/* Network is unreachable */
#define	ENETRESET		52		/* Network dropped connection on reset */
#define	ECONNABORTED	53		/* Software caused connection abort */
#define	ECONNRESET		54		/* Connection reset by peer */
#define	ENOBUFS			55		/* No buffer space available */
#define	EISCONN			56		/* Socket is already connected */
#define	ENOTCONN		57		/* Socket is not connected */
#define	ESHUTDOWN		58		/* Can't send after socket shutdown */
#define	ETOOMANYREFS	59		/* Too many references: can't splice */
#define	ETIMEDOUT		60		/* Connection timed out */
#define	ECONNREFUSED	61		/* Connection refused */

/* */
#define	ELOOP			62		/* Too many levels of symbolic links */
#define	ENAMETOOLONG	63		/* File name too long */

/* should be rearranged */
#define	EHOSTDOWN		64		/* Host is down */
#define	EHOSTUNREACH	65		/* No route to host */
#define	ENOTEMPTY		66		/* Directory not empty */

/* quotas & mush */
#define	EPROCLIM		67		/* Too many processes */
#define	EUSERS			68		/* Too many users */
#define	EDQUOT			69		/* Disc quota exceeded */

/* Network File System */
#define	ESTALE			70		/* Stale NFS file handle */
#define	EREMOTE			71		/* Too many levels of remote in path */
#define	EBADRPC			72		/* RPC struct is bad */
#define	ERPCMISMATCH	73		/* RPC version wrong */
#define	EPROGUNAVAIL	74		/* RPC prog. not avail */
#define	EPROGMISMATCH	75		/* Program version wrong */
#define	EPROCUNAVAIL	76		/* Bad procedure for program */

#define	ENOLCK			77		/* No locks available */
#define	ENOSYS			78		/* Function not implemented */

#define	EFTYPE			79		/* Inappropriate file type or format */
#define	EAUTH			  80		/* Authentication error */
#define	ENEEDAUTH		81		/* Need authenticator */

/* SystemV IPC */
#define	ENOMSG		    82		/* No message of desired type */
#define	EOVERFLOW	    83		/* Value too large to be stored in data type */

/* Wide/multibyte-character handling, ISO/IEC 9899/AMD1:1995 */
#define	EILSEQ		    84		/* Illegal byte sequence */

/* From IEEE Std 1003.1-2001 */
/* Base, Realtime, Threads or Thread Priority Scheduling option errors */
#define ENOTSUP		    85		/* Not supported */

/* Realtime option errors */
#define ECANCELED	    86		/* Operation canceled */

#define	ELAST			    86		/* Must be equal largest errno */

#ifdef	_KERNEL
/* pseudo-errors returned inside kernel to modify return back to user mode */
#define	ERESTART		  -1		/* restart syscall */
#define	EJUSTRETURN		-2		/* don't modify regs, just return */
#define	ENOIOCTL		  -3		/* ioctl not handled by this layer (aka EPASSTHROUGH) */
#define EPASSTHROUGH	ENOIOCTL
#endif

#endif /* _SYS_ERRNO_H_ */
