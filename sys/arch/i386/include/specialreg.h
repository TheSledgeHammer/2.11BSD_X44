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

#define	CR3_PCID_SAVE 	0x8000000000000000
#define	CR3_PCID_MASK 	0xfff

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
#define	CR4_FXSR 		0x00000200	/* Fast FPU save/restore used by OS */
#define	CR4_XMM			0x00000400	/* enable SIMD/MMX2 to use except 16 */
#define	CR4_UMIP 		0x00000800	/* User Mode Instruction Prevention */
#define	CR4_LA57 		0x00001000	/* Enable 5-level paging */
#define	CR4_VMXE 		0x00002000	/* enable VMX operation (Intel-specific) */
#define	CR4_FSGSBASE 	0x00010000	/* Enable FS/GS BASE accessing instructions */
#define	CR4_PCIDE 		0x00020000	/* Enable Context ID */
#define	CR4_XSAVE 		0x00040000	/* XSETBV/XGETBV */
#define	CR4_SMEP 		0x00100000	/* Supervisor-Mode Execution Prevention */
#define	CR4_SMAP 		0x00200000	/* Supervisor-Mode Access Prevention */
#define	CR4_PKE			0x00400000	/* Protection Keys Enable */

/*
 * CPUID instruction features register
 */
#define	CPUID_FPU		0x00000001
#define	CPUID_VME		0x00000002
#define	CPUID_DE		0x00000004
#define	CPUID_PSE		0x00000008
#define	CPUID_TSC		0x00000010
#define	CPUID_MSR		0x00000020
#define	CPUID_PAE		0x00000040
#define	CPUID_MCE		0x00000080
#define	CPUID_CX8		0x00000100
#define	CPUID_APIC		0x00000200
#define	CPUID_B10		0x00000400
#define	CPUID_SEP		0x00000800
#define	CPUID_MTRR		0x00001000
#define	CPUID_PGE		0x00002000
#define	CPUID_MCA		0x00004000
#define	CPUID_CMOV		0x00008000
#define	CPUID_PAT		0x00010000
#define	CPUID_PSE36		0x00020000
#define	CPUID_PSN		0x00040000
#define	CPUID_CLFSH		0x00080000
#define	CPUID_B20		0x00100000
#define	CPUID_DS		0x00200000
#define	CPUID_ACPI		0x00400000
#define	CPUID_MMX		0x00800000
#define	CPUID_FXSR		0x01000000
#define	CPUID_SSE		0x02000000
#define	CPUID_XMM		0x02000000
#define	CPUID_SSE2		0x04000000
#define	CPUID_SS		0x08000000
#define	CPUID_HTT		0x10000000
#define	CPUID_TM		0x20000000
#define	CPUID_IA64		0x40000000
#define	CPUID_PBE		0x80000000

#define	CPUID2_SSE3		0x00000001
#define	CPUID2_PCLMULQDQ 0x00000002
#define	CPUID2_DTES64	0x00000004
#define	CPUID2_MON		0x00000008
#define	CPUID2_DS_CPL	0x00000010
#define	CPUID2_VMX		0x00000020
#define	CPUID2_SMX		0x00000040
#define	CPUID2_EST		0x00000080
#define	CPUID2_TM2		0x00000100
#define	CPUID2_SSSE3	0x00000200
#define	CPUID2_CNXTID	0x00000400
#define	CPUID2_SDBG		0x00000800
#define	CPUID2_FMA		0x00001000
#define	CPUID2_CX16		0x00002000
#define	CPUID2_XTPR		0x00004000
#define	CPUID2_PDCM		0x00008000
#define	CPUID2_PCID		0x00020000
#define	CPUID2_DCA		0x00040000
#define	CPUID2_SSE41	0x00080000
#define	CPUID2_SSE42	0x00100000
#define	CPUID2_X2APIC	0x00200000
#define	CPUID2_MOVBE	0x00400000
#define	CPUID2_POPCNT	0x00800000
#define	CPUID2_TSCDLT	0x01000000
#define	CPUID2_AESNI	0x02000000
#define	CPUID2_XSAVE	0x04000000
#define	CPUID2_OSXSAVE	0x08000000
#define	CPUID2_AVX		0x10000000
#define	CPUID2_F16C		0x20000000
#define	CPUID2_RDRAND	0x40000000
#define	CPUID2_HV		0x80000000

/*
 * CPUID manufacturers identifiers
 */
#define	AMD_VENDOR_ID		"AuthenticAMD"
#define	CENTAUR_VENDOR_ID	"CentaurHauls"
#define	CYRIX_VENDOR_ID		"CyrixInstead"
#define	INTEL_VENDOR_ID		"GenuineIntel"
#define	NEXGEN_VENDOR_ID	"NexGenDriven"
#define	NSC_VENDOR_ID		"Geode by NSC"
#define	RISE_VENDOR_ID		"RiseRiseRise"
#define	SIS_VENDOR_ID		"SiS SiS SiS "
#define	TRANSMETA_VENDOR_ID	"GenuineTMx86"
#define	UMC_VENDOR_ID		"UMC UMC UMC "
#define	HYGON_VENDOR_ID		"HygonGenuine"

/*
 * CPUID instruction 1 ebx info
 */
#define	CPUID_BRAND_INDEX		0x000000ff
#define	CPUID_CLFUSH_SIZE		0x0000ff00
#define	CPUID_HTT_CORES			0x00ff0000
#define	CPUID_LOCAL_APIC_ID		0xff000000

/*
 * CPUID instruction 5 info
 */
#define	CPUID5_MON_MIN_SIZE		0x0000ffff	/* eax */
#define	CPUID5_MON_MAX_SIZE		0x0000ffff	/* ebx */
#define	CPUID5_MON_MWAIT_EXT	0x00000001	/* ecx */
#define	CPUID5_MWAIT_INTRBREAK	0x00000002	/* ecx */

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
#define	CCR1_SMI		0x02	/* Enables SMM pins */
#define	CCR1_SMAC		0x04	/* System management memory access */
#define	CCR1_MMAC		0x08	/* Main memory access */
#define	CCR1_NO_LOCK	0x10	/* Negate LOCK# */
#define	CCR1_SM3		0x80	/* SMM address space address region 3 */

#define	CCR2			0xc2
#define	CCR2_WB			0x02	/* Enables WB cache interface pins */
#define	CCR2_SADS		0x02	/* Slow ADS */
#define	CCR2_LOCK_NW	0x04	/* LOCK NW Bit */
#define	CCR2_SUSP_HLT	0x08	/* Suspend on HALT */
#define	CCR2_WT1		0x10	/* WT region 1 */
#define	CCR2_WPR1		0x10	/* Write-protect region 1 */
#define	CCR2_BARB		0x20	/* Flushes write-back cache when entering
								   hold state. */
#define	CCR2_BWRT		0x40	/* Enables burst write cycles */
#define	CCR2_USE_SUSP	0x80	/* Enables suspend pins */

#define	CCR3			0xc3
#define	CCR3_SMILOCK	0x01	/* SMM register lock */
#define	CCR3_NMI		0x02	/* Enables NMI during SMM */
#define	CCR3_LINBRST	0x04	/* Linear address burst cycles */
#define	CCR3_SMMMODE	0x08	/* SMM Mode */
#define	CCR3_MAPEN0		0x10	/* Enables Map0 */
#define	CCR3_MAPEN1		0x20	/* Enables Map1 */
#define	CCR3_MAPEN2		0x40	/* Enables Map2 */
#define	CCR3_MAPEN3		0x80	/* Enables Map3 */

#define	CCR4			0xe8
#define	CCR4_IOMASK		0x07
#define	CCR4_MEM		0x08	/* Enables momory bypassing */
#define	CCR4_DTE		0x10	/* Enables directory table entry cache */
#define	CCR4_FASTFPE	0x20	/* Fast FPU exception */
#define	CCR4_CPUID		0x80	/* Enables CPUID instruction */

#define	CCR5			0xe9
#define	CCR5_WT_ALLOC	0x01	/* Write-through allocate */
#define	CCR5_SLOP		0x02	/* LOOP instruction slowed down */
#define	CCR5_LBR1		0x10	/* Local bus region 1 */
#define	CCR5_ARREN		0x20	/* Enables ARR region */

#define	CCR6			0xea

#define	CCR7			0xeb

/* Performance Control Register (5x86 only). */
#define	PCR0			0x20
#define	PCR0_RSTK		0x01	/* Enables return stack */
#define	PCR0_BTB		0x02	/* Enables branch target buffer */
#define	PCR0_LOOP		0x04	/* Enables loop */
#define	PCR0_AIS		0x08	/* Enables all instrcutions stalled to
								   serialize pipe. */
#define	PCR0_MLR		0x10	/* Enables reordering of misaligned loads */
#define	PCR0_BTBRT		0x40	/* Enables BTB test register. */
#define	PCR0_LSSER		0x80	/* Disable reorder */

/* Device Identification Registers */
#define	DIR0			0xfe
#define	DIR1			0xff


/*
 * Important bits in the AMD extended cpuid flags
 */
#define	AMDID_LM				0x20000000

/*
 * AMD extended function 8000_0008h ebx info (amd_extended_feature_extensions)
 */
#define	AMDFEID_CLZERO			0x00000001
#define	AMDFEID_IRPERF			0x00000002
#define	AMDFEID_XSAVEERPTR		0x00000004
#define	AMDFEID_RDPRU			0x00000010
#define	AMDFEID_MCOMMIT			0x00000100
#define	AMDFEID_WBNOINVD		0x00000200
#define	AMDFEID_IBPB			0x00001000
#define	AMDFEID_IBRS			0x00004000
#define	AMDFEID_STIBP			0x00008000
/* The below are only defined if the corresponding base feature above exists. */
#define	AMDFEID_IBRS_ALWAYSON	0x00010000
#define	AMDFEID_STIBP_ALWAYSON	0x00020000
#define	AMDFEID_PREFER_IBRS		0x00040000
#define	AMDFEID_PPIN			0x00800000
#define	AMDFEID_SSBD			0x01000000
/* SSBD via MSRC001_011F instead of MSR 0x48: */
#define	AMDFEID_VIRT_SSBD		0x02000000
#define	AMDFEID_SSB_NO			0x04000000

#endif /* !_MACHINE_SPECIALREG_H_ */
