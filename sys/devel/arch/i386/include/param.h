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
#define	NBSEG			4194304				/* bytes/segment (SEGMENT SIZE) */

#define	SEGOFSET		(NBSEG-1)			/* byte offset into segment */
#define	SEGSHIFT		22					/* LOG2(NBSEG) */
#define	SEGSIZE			(1 << SEGSHIFT)		/* bytes/segment (SEGMENT SIZE) */
#define SEGMASK			SEGOFSET			/* SEGOFSET (SEGSIZE - 1) */

/* pages */
#define	NBPG			4096				/* bytes/page (PAGE SIZE) */

#define	PGOFSET			(NBPG-1)			/* byte offset into page */
#define	PGSHIFT			12					/* LOG2(NBPG) */
#define PGSIZE			(1 << PGSHIFT)		/* bytes/page (PAGE SIZE) */
#define PGMASK			PGOFSET				/* PGOFSET (PGSIZE - 1) */

#define	NPTEPG			(NBPG / sizeof(pt_entry_t))
/* Size in bytes of the page directory */
#define NBPTD			(NPGPTD << PGSHIFT)
/* Number of PDEs in page directory, 2048 for PAE, 1024 for non-PAE */
#define NPDEPTD			(NBPTD / sizeof(pd_entry_t))
/* Number of PDEs in one page of the page directory, 512 vs. 1024 */
#define NPDEPG			(NBPG / sizeof(pd_entry_t))

#define	KERNBASE		0xFE000000			/* start of kernel virtual (i.e. SYSTEM) */
#define KERNLOAD		KERNBASE			/* Kernel physical load address */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)


#endif /* _I386_PARAM_H_ */
