/*-
 * Copyright (c) 1982, 1986, 1993
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
 *
 *	@(#)mman.h	8.1 (Berkeley) 6/2/93
 * $Id: mman.h,v 1.3 1994/08/02 15:06:58 davidg Exp $
 */

#ifndef _SYS_MMAN_H_
#define _SYS_MMAN_H_

#include <machine/ansi.h>

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#include <sys/ansi.h>
#include <sys/types.h>

#ifndef	off_t
typedef	__off_t		off_t;		/* file offset */
#define	off_t		__off_t
#endif

/*
 * Protections are chosen from these bits, or-ed together
 */
#define	PROT_NONE	        0x00	/* no permissions */
#define	PROT_READ			    0x01	/* pages can be read */
#define	PROT_WRITE			  0x02	/* pages can be written */
#define	PROT_EXEC			    0x04	/* pages can be executed */

/*
 * Flags contain sharing type and options.
 * Sharing types; choose one.
 */
#define	MAP_SHARED			  0x0001	/* share changes */
#define	MAP_PRIVATE			  0x0002	/* changes are private */
#define	MAP_COPY			    0x0004	/* "copy" region at mmap time */

/*
 * Other flags
 */
#define	MAP_FIXED	 		    0x0010	/* map addr must be exactly as requested */
#define	MAP_RENAME	 		  0x0020	/* Sun: rename private pages to file */
#define	MAP_NORESERVE		  0x0040	/* Sun: don't reserve needed swap area */
#define	MAP_INHERIT	 		  0x0080	/* region is retained after exec */
#define	MAP_NOEXTEND	 	  0x0100	/* for MAP_FILE, don't change file size */
#define	MAP_HASSEMAPHORE 	0x0200	/* region may contain semaphores */

/*
 * Mapping type; default is map from file.
 */
#define MAP_FILE			    0x0000	/* for backward source compatibility */
#define	MAP_ANON			    0x1000	/* allocated from memory, swap space */
#define	MAP_ANONYMOUS           MAP_ANON

/*
 * Alignment (expressed in log2).  Must be >= log2(PAGE_SIZE) and
 * < # bits in a pointer (32 or 64).
 */
#define	MAP_ALIGNED(n)	    ((int)((unsigned int)(n) << MAP_ALIGNMENT_SHIFT))
#define	MAP_ALIGNMENT_SHIFT	24

/*
 * Inherit to minherit
 */
#define	MAP_INHERIT_SHARE		0
#define	MAP_INHERIT_COPY		1
#define	MAP_INHERIT_NONE		2
#define MAP_INHERIT_DONATE_COPY		3
#define	MAP_INHERIT_ZERO		4

/*
 * Advice to madvise
 */
#define	MADV_NORMAL			0		/* no further special treatment */
#define	MADV_RANDOM			1		/* expect random page references */
#define	MADV_SEQUENTIAL			2		/* expect sequential page references */
#define	MADV_WILLNEED			3		/* will need these pages */
#define	MADV_DONTNEED			4		/* dont need these pages */
#define MADV_SPACEAVAIL			5		/* Insure that resources are reserved */
#define MADV_FREE			6		/* Pages are empty, free them */

/*
 * Error return from mmap()
 */
#define MAP_FAILED			((void *)-1)

/*
 * Flags to msync
 */
#define	MS_ASYNC			0x01	/* perform asynchronous writes */
#define	MS_INVALIDATE			0x02	/* invalidate cached data */
#define	MS_SYNC				0x04	/* perform synchronous writes */

#ifndef _KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
/* Some of these int's should probably be size_t's */
caddr_t	mmap(caddr_t, size_t, int, int, int, off_t);
int	mprotect(caddr_t, size_t, int);
int	munmap(caddr_t, size_t);
int	msync(caddr_t, size_t, int);
int	mlock(caddr_t, size_t);
int	munlock(caddr_t, size_t);
int 	madvise(caddr_t, size_t, int);
int	minherit(caddr_t, size_t, int);
__END_DECLS

#endif /* !KERNEL */
#endif
