/*-
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

#ifndef _AMD64_VMPARAM_H_
#define _AMD64_VMPARAM_H_

#include <arch/amd64/include/param.h>
#include <arch/amd64/include/pmap.h>

#define L4_DMAP_SLOTS 		1	/* number of dmap PML4 slots (i.e. NDMPML4E)  */

#define	KPML4I				(L4_SLOT_KERNBASE)	/* Top 512GB for KVM */
#define	DMPML4I				(L4_SLOT_APTE)

#define L4_SLOT_DIRECT		L4_SLOT_APTE		/* Next 512GB down for direct map */
#define PDIR_SLOT_DIRECT	L4_SLOT_DIRECT

#define	DMAP_MIN_ADDRESS	K4VADDR(PDIR_SLOT_DIRECT, 0, 0, 0)
#define	DMAP_MAX_ADDRESS	K4VADDR(PDIR_SLOT_DIRECT+1, 0, 0, 0)

#define	PHYS_TO_DMAP(x)		((x) | DMAP_MIN_ADDRESS)
#define	DMAP_TO_PHYS(x)		((x) & ~DMAP_MIN_ADDRESS)

#endif /* _AMD64_VMPARAM_H_ */
