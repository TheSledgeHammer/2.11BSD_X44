/*-
 * Copyright (c) 1991, 1993
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
 *	@(#)specialreg.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _MACHINE_SPECIALREG_H_
#define	_MACHINE_SPECIALREG_H_

/*
 * 386 Special registers:
 */
#define	CR0_PE			0x00000001	/* Protected mode Enable */
#define	CR0_MP			0x00000002	/* "Math" Present (e.g. npx), wait for it */
#define	CR0_EM			0x00000004	/* EMulate NPX, e.g. trap, don't execute code */
#define	CR0_TS			0x00000008	/* Process has done Task Switch, do NPX save */
#define	CR0_ET			0x00000010	/* 32 bit (if set) vs 16 bit (387 vs 287) */
#define	CR0_PG			0x80000000	/* Paging Enable */

/*
 * Bits in 486 special registers:
 */
#define	CR0_NE			0x00000020	/* Numeric Error enable (EX16 vs IRQ13) */
#define	CR0_WP			0x00010000	/* Write Protect (honor page protect in all modes) */
#define	CR0_AM			0x00040000	/* Alignment Mask (set to enable AC flag) */
#define	CR0_NW  		0x20000000	/* Not Write-through */
#define	CR0_CD  		0x40000000	/* Cache Disable */

/*
 * Cyrix 486 DLC special registers, accessible as IO ports.
 */
#define CCR0			0xc0	/* configuration control register 0 */
#define CCR0_NC0		0x01	/* first 64K of each 1M memory region is non-cacheable */
#define CCR0_NC1		0x02	/* 640K-1M region is non-cacheable */
#define CCR0_A20M		0x04	/* enables A20M# input pin */
#define CCR0_KEN		0x08	/* enables KEN# input pin */
#define CCR0_FLUSH		0x10	/* enables FLUSH# input pin */
#define CCR0_BARB		0x20	/* flushes internal cache when entering hold state */
#define CCR0_CO			0x40	/* cache org: 1=direct mapped, 0=2x set assoc */
#define CCR0_SUSPEND	0x80	/* enables SUSP# and SUSPA# pins */

#define CCR1			0xc1	/* configuration control register 1 */
#define CCR1_RPL		0x01	/* enables RPLSET and RPLVAL# pins */
/* the remaining 7 bits of this register are reserved */

/*
 * bits in the pentiums %cr4 register:
 */
#define CR4_VME			0x00000001	/* virtual 8086 mode extension enable */
#define CR4_PVI 		0x00000002	/* protected mode virtual interrupt enable */
#define CR4_TSD 		0x00000004	/* restrict RDTSC instruction to cpl 0 only */
#define CR4_DE			0x00000008	/* debugging extension */
#define CR4_PSE			0x00000010	/* large (4MB) page size enable */
#define CR4_PAE 		0x00000020	/* physical address extension enable */
#define CR4_MCE			0x00000040	/* machine check enable */
#define CR4_PGE			0x00000080	/* page global enable */
#define CR4_PCE			0x00000100	/* enable RDPMC instruction for all cpls */
#define CR4_OSFXSR		0x00000200	/* enable fxsave/fxrestor and SSE */
#define CR4_OSXMMEXCPT	0x00000400	/* enable unmasked SSE exceptions */

/*
 * CPUID instruction features register
 */
#define	CPUID_VME		0x00000002

#endif /* !_MACHINE_SPECIALREG_H_ */
