/*
 * Copyright (c) 1990, 1993
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
 *	@(#)filedesc.h	8.1 (Berkeley) 6/2/93
 * $Id: filedesc.h,v 1.3 1994/08/21 04:41:46 paul Exp $
 */

#ifndef _SYS_FILEDESC_H_
#define _SYS_FILEDESC_H_

#include <sys/lock.h>

/*
 * This structure is used for the management of descriptors.  It may be
 * shared by multiple processes.
 *
 * A process is initially started out with NDFILE descriptors stored within
 * this structure, selected to be enough for typical applications based on
 * the historical limit of 20 open files (and the usage of descriptors by
 * shells).  If these descriptors are exhausted, a larger descriptor table
 * may be allocated, up to a process' resource limit; the internal arrays
 * are then unused.  The initial expansion is set to NDEXTENT; each time
 * it runs out, it is doubled until the resource limit is reached. NDEXTENT
 * should be selected to be the biggest multiple of OFILESIZE (see below)
 * that will fit in a power-of-two sized piece of memory.
 */
#define NDFILE			20
#define NDEXTENT		50					/* 250 bytes in 256-byte alloc. */

struct filedesc {
	struct	file 		**fd_ofiles;		/* file structures for open files */
	char				*fd_ofileflags;		/* per-process open file flags */
	struct	vnode 		*fd_cdir;			/* current directory */
	struct	vnode 		*fd_rdir;			/* root directory */
	int					fd_nfiles;			/* number of open files allocated */
	int				    fd_lastfile;		/* high-water mark of fd_ofiles */
	int				    fd_freefile;		/* approx. next free file */
	u_short				fd_cmask;			/* mask for file creation */
	u_short				fd_refcnt;			/* reference count */
	struct lock_object 	fd_slock;

	int					fd_knlistsize;		/* size of fd_knlist */
	struct klist		*fd_knlist;			/* list of attached fd knotes, indexed by fd number */
	u_long				fd_knhashmask;		/* size of fd_knhash */
	struct klist		*fd_knhash;			/* hash table for attached non-fd knotes */
};

/*
 * Basic allocation of descriptors:
 * one of the above, plus arrays for NDFILE descriptors.
 */
struct filedesc0 {
	struct	filedesc 	fd_fd;
	/*
	 * These arrays are used when the number of open files is
	 * <= NDFILE, and are then pointed to by the pointers above.
	 */
	struct	file 		*fd_dfiles[NDFILE];
	char				fd_dfileflags[NDFILE];
};

/*
 * Per-process open flags.
 */
#define	UF_EXCLOSE 		0x01				/* auto-close on exec */
#define	UF_MAPPED 		0x02				/* mapped from device */

/*
 * Storage required per open file descriptor.
 */
#define OFILESIZE (sizeof(struct file *) + sizeof(char))

#ifdef _KERNEL
/*
 * Kernel global variables and routines.
 */ 
void				finit(struct filedesc *);
int			        ufalloc(int, int *);
struct file	        *falloc(void);
void				fdexpand(int);
int  		        ufdalloc(struct file *);
struct filedesc 	*fdalloc(struct file *);
int 				fdavail(int);
void				ffree(struct file *);
struct file 		*getf(int);
int					closef(struct file *);
int					fdopen(dev_t, int, int);
int                 dupfdopen(struct filedesc *, int, int, int, int);
void				fdfree(struct filedesc *);
int					fdclone(struct file *, int, int, struct fileops *, void *);
struct filedesc 	*fdcopy(struct filedesc *);
void 				ufdsync(struct filedesc *);
void 				fdsync(struct filedesc *);
struct file 		*fd_getfile(int);
void				fdremove(int);
int					fdrelease(int);
void				fdunshare(void);
void				fdcloseexec(void);
int					getfiledesc(struct filedesc *, int, struct file **, int);
int					checkfiledesc(struct file **, int);
void				fownsignal(pid_t, int);
#endif
#endif
