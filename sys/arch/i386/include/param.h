/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)param.h	8.3 (Berkeley) 5/14/95
 */

#ifndef	_I386_PARAM_H_
#define	_I386_PARAM_H_

/*
 * Machine dependent constants for Intel 386.
 */

#ifdef _KERNEL
#ifdef _LOCORE
#include <machine/psl.h>
#else
#include <machine/cpu.h>
#endif
#endif

#ifndef MACHINE
#define MACHINE			"i386"
#endif
#ifndef MACHINE_ARCH
#define	MACHINE_ARCH	"i386"
#endif
#define MID_MACHINE		MID_I386

/* Maximum Number of CPU's */
#ifdef SMP
#ifndef NCPUS
#define NCPUS 			32
#endif
#else
#define NCPUS 			1
#endif

/*
 * Round p (pointer or byte index) up to a correctly-aligned value for all
 * data types (int, long, ...).   The result is u_int and must be cast to
 * any desired pointer type.
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture
 * using ALIGNED_POINTER_LOAD.  This does not reflect the optimal
 * alignment, just the possibility (within reasonable limits).
 */

#define	_ALIGNBYTES				(sizeof(int) - 1)
#define	_ALIGN(p)				(((unsigned long)(p) + _ALIGNBYTES) &~ _ALIGNBYTES)
#define	_ALIGNED_POINTER(p,t)	((((unsigned long)(p)) & (__alignof(t) - 1)) == 0)

/* segments */
#define	NBSG			4194304					/* bytes/segment (SEGMENT SIZE) */

#define	SGOFSET			(NBSG-1)				/* byte offset into segment */
#define	SGSHIFT			22						/* LOG2(NBSG) */
#define	SGSIZE			(1 << SGSHIFT)			/* bytes/segment (SEGMENT SIZE) */
#define SGMASK			SGOFSET					/* SGOFSET (SGSIZE - 1) */

/* pages */
#define	NBPG			4096					/* bytes/page (PAGE SIZE) */

#define	PGOFSET			(NBPG-1)				/* byte offset into page */
#define	PGSHIFT			12						/* LOG2(NBPG) */
#define PGSIZE			(1 << PGSHIFT)			/* bytes/page (PAGE SIZE) */
#define PGMASK			PGOFSET					/* PGOFSET (PGSIZE - 1) */

/* Number of pages needed for overlay space */
#define NPGOVL  		129

#define	NPTEPG			(NBPG / sizeof(pt_entry_t))
/* Size in bytes of the page directory */
#define NBPTD			(NPGPTD << PGSHIFT)
/* Number of PDEs in page directory, 2048 for PAE, 1024 for non-PAE */
#define NPDEPTD			(NBPTD / sizeof(pd_entry_t))
/* Number of PDEs in one page of the page directory, 512 vs. 1024 */
#define NPDEPG			(NBPG / sizeof(pd_entry_t))

#define	KERNBASE		0xC0000000UL			/* start of kernel virtual (i.e. SYSTEM) */
#define KERNLOAD		(KERNBASE + 0x100000)	/* Kernel physical load address */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define	DEV_BSIZE		512
#define	DEV_BSHIFT		9						/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define	MAXPHYS			(64 * 1024)				/* max raw I/O transfer size */

#define	CLSIZE			1
#define	CLSIZELOG2		0

/* NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE */
#define	SSIZE			1						/* initial stack size/NBPG */
#define	SINCR			1						/* increment of stack/NBPG */

#define	UPAGES			2						/* pages of u-area */
#define	USPACE			(UPAGES * PGSIZE)		/* total size of u-area */

#ifndef KSTACK_PAGES
#define KSTACK_PAGES 	4						/* Includes pcb! */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than CLBYTES (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE			128						/* size of an mbuf */
#define	MCLBYTES		1024
#define	MCLSHIFT		10
#define	MCLOFSET		(MCLBYTES - 1)
#ifndef NMBCLUSTERS
#ifdef GATEWAY
#define	NMBCLUSTERS		512						/* map size, max cluster allocation */
#else
#define	NMBCLUSTERS		256						/* map size, max cluster allocation */
#endif
#endif

/*
 * Size of kernel malloc arena in CLBYTES-sized logical pages
 */
#ifndef NKMEMCLUSTERS
#define	NKMEMCLUSTERS	(2048*1024/CLBYTES)
#endif

/*
 * Some macros for units conversion
 */
/* Core clicks (4096 bytes) to segments and vice versa */
#define	ctos(x)			(x)	//((x)<<SGSHIFT)
#define	stoc(x)			(x) //(((unsigned)(x)+(SGOFSET))>>SGSHIFT)

/* Core clicks (4096 bytes) to disk blocks */
#define	ctod(x)			((x)<<(PGSHIFT-DEV_BSHIFT))
#define	dtoc(x)			((x)>>(PGSHIFT-DEV_BSHIFT))
#define	dtob(x)			((x)<<DEV_BSHIFT)

/* clicks to bytes */
#define	ctob(x)			((x)<<PGSHIFT)

/* bytes to clicks */
#define	btoc(x)			(((unsigned)(x)+(PGOFSET))>>PGSHIFT)

#define	btodb(bytes)	((bytes) >> DEV_BSHIFT)		/* calculates (bytes / DEV_BSIZE) */

#define	dbtob(db)		((db) << DEV_BSHIFT)		/* calculates (db * DEV_BSIZE) */

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and will be if we
 * add an entry to cdevsw/bdevsw for that purpose.
 * For now though just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * Mach derived conversion macros
 */
#define i386_round_pdr(x)		((((unsigned)(x)) + NBPDR - 1) & ~(NBPDR-1))
#define i386_trunc_pdr(x)		((unsigned)(x) & ~(NBPDR-1))
#define i386_btod(x)			((unsigned)(x) >> PDRSHIFT)
#define i386_dtob(x)			((unsigned)(x) << PDRSHIFT)

#define i386_round_segment(x)	((((unsigned)(x)) + NBSG - 1) & ~(NBSG-1))
#define i386_trunc_segment(x)	((unsigned)(x) & ~(NBSG-1))
#define i386_round_page(x)		((((unsigned)(x)) + NBPG - 1) & ~(NBPG-1))
#define i386_trunc_page(x)		((unsigned)(x) & ~(NBPG-1))
#define i386_btos(x)			((unsigned)(x) >> SGSHIFT)
#define i386_stob(x)			((unsigned)(x) << SGSHIFT)
#define i386_btop(x)			((unsigned)(x) >> PGSHIFT)
#define i386_ptob(x)			((unsigned)(x) << PGSHIFT)

#endif /* _I386_PARAM_H_ */
