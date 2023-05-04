/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2003 Peter Wemm.
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and William Jolitz of UUNET Technologies Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * Derived from hp300 version by Mike Hibler, this version by William
 * Jolitz uses a recursive map [a pde points to the page directory] to
 * map the page tables using the pagetables themselves. This is done to
 * reduce the impact on kernel virtual memory for lots of sparse address
 * space, and to reduce the cost of memory to each process.
 *
 *	from: hp300: @(#)pmap.h	7.2 (Berkeley) 12/16/90
 *	from: @(#)pmap.h	7.4 (Berkeley) 5/12/91
 * $FreeBSD$
 */

#ifndef _AMD64_PTE_H_
#define _AMD64_PTE_H_

#include <arch/i386/include/pte.h>

/*
 * Page-directory and page-table entries follow this format, with a few
 * of the fields not present here and there, depending on a lot of things.
 */
#define	PG_PKU(idx)				((pt_entry_t)idx << 59)
#define	PG_AVAIL(x)				(1UL << (x))

/*
 * Intel extended page table (EPT) bit definitions.
 */
#define	EPT_PG_READ				0x001		/* R	Read		*/
#define	EPT_PG_WRITE			0x002		/* W	Write		*/
#define	EPT_PG_EXECUTE			0x004		/* X	Execute		*/
#define	EPT_PG_IGNORE_PAT		0x040		/* IPAT	Ignore PAT	*/
#define	EPT_PG_PS				0x080		/* PS	Page size	*/
#define	EPT_PG_A				0x100		/* A	Accessed	*/
#define	EPT_PG_M				0x200		/* D	Dirty		*/
#define	EPT_PG_MEMORY_TYPE(x)	((x) << 3) 	/* MT Memory Type	*/

/* Our various interpretations of the above */
#define	EPT_PG_EMUL_V			PG_AVAIL(52)
#define	EPT_PG_EMUL_RW			PG_AVAIL(53)
#define	PG_PROMOTED				PG_AVAIL(54)/* PDE only */

#endif /* _AMD64_PTE_H_ */
