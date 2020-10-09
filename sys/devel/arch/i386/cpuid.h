/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
 *	from: @(#)specialreg.h	7.1 (Berkeley) 5/9/91
 * $FreeBSD$
 */

#ifndef _MACHINE_CPUID_H_
#define	_MACHINE_CPUID_H_

/*
 * Bits in AMD64 special registers.  EFER is 64 bits wide.
 */
#define	EFER_SCE 	0x000000001	/* System Call Extensions (R/W) */
#define	EFER_LME 	0x000000100	/* Long mode enable (R/W) */
#define	EFER_LMA 	0x000000400	/* Long mode active (R) */
#define	EFER_NXE 	0x000000800	/* PTE No-Execute bit enable (R/W) */
#define	EFER_SVM 	0x000001000	/* SVM enable bit for AMD, reserved for Intel */
#define	EFER_LMSLE 	0x000002000	/* Long Mode Segment Limit Enable */
#define	EFER_FFXSR 	0x000004000	/* Fast FXSAVE/FSRSTOR */
#define	EFER_TCE   	0x000008000	/* Translation Cache Extension */

/*
 * Intel Extended Features registers
 */
#define	XCR0						0		/* XFEATURE_ENABLED_MASK register */

#define	XFEATURE_ENABLED_X87		0x00000001
#define	XFEATURE_ENABLED_SSE		0x00000002
#define	XFEATURE_ENABLED_YMM_HI128	0x00000004
#define	XFEATURE_ENABLED_AVX		XFEATURE_ENABLED_YMM_HI128
#define	XFEATURE_ENABLED_BNDREGS	0x00000008
#define	XFEATURE_ENABLED_BNDCSR		0x00000010
#define	XFEATURE_ENABLED_OPMASK		0x00000020
#define	XFEATURE_ENABLED_ZMM_HI256	0x00000040
#define	XFEATURE_ENABLED_HI16_ZMM	0x00000080

#define	XFEATURE_AVX										\
    (XFEATURE_ENABLED_X87 | XFEATURE_ENABLED_SSE | XFEATURE_ENABLED_AVX)
#define	XFEATURE_AVX512										\
    (XFEATURE_ENABLED_OPMASK | XFEATURE_ENABLED_ZMM_HI256 |	\
    XFEATURE_ENABLED_HI16_ZMM)
#define	XFEATURE_MPX										\
    (XFEATURE_ENABLED_BNDREGS | XFEATURE_ENABLED_BNDCSR)

/*
 * Important bits in the Thermal and Power Management flags
 * CPUID.6 EAX and ECX.
 */
#define	CPUTPM1_SENSOR				0x00000001
#define	CPUTPM1_TURBO				0x00000002
#define	CPUTPM1_ARAT				0x00000004
#define	CPUTPM1_HWP					0x00000080
#define	CPUTPM1_HWP_NOTIFICATION	0x00000100
#define	CPUTPM1_HWP_ACTIVITY_WINDOW	0x00000200
#define	CPUTPM1_HWP_PERF_PREF		0x00000400
#define	CPUTPM1_HWP_PKG				0x00000800
#define	CPUTPM1_HWP_FLEXIBLE		0x00020000
#define	CPUTPM2_EFFREQ				0x00000001

/* Intel Processor Trace CPUID. */

/* Leaf 0 ebx. */
#define	CPUPT_CR3			(1 << 0)	/* CR3 Filtering Support */
#define	CPUPT_PSB			(1 << 1)	/* Configurable PSB and Cycle-Accurate Mode Supported */
#define	CPUPT_IPF			(1 << 2)	/* IP Filtering and TraceStop supported */
#define	CPUPT_MTC			(1 << 3)	/* MTC Supported */
#define	CPUPT_PRW			(1 << 4)	/* PTWRITE Supported */
#define	CPUPT_PWR			(1 << 5)	/* Power Event Trace Supported */

/* Leaf 0 ecx. */
#define	CPUPT_TOPA			(1 << 0)	/* ToPA Output Supported */
#define	CPUPT_TOPA_MULTI	(1 << 1)	/* ToPA Tables Allow Multiple Output Entries */
#define	CPUPT_SINGLE		(1 << 2)	/* Single-Range Output Supported */
#define	CPUPT_TT_OUT		(1 << 3)	/* Output to Trace Transport Subsystem Supported */
#define	CPUPT_LINEAR_IP		(1 << 31)	/* IP Payloads are Linear IP, otherwise IP is effective */

/* Leaf 1 eax. */
#define	CPUPT_NADDR_S		0	/* Number of Address Ranges */
#define	CPUPT_NADDR_M		(0x7 << CPUPT_NADDR_S)
#define	CPUPT_MTC_BITMAP_S	16	/* Bitmap of supported MTC Period Encodings */
#define	CPUPT_MTC_BITMAP_M	(0xffff << CPUPT_MTC_BITMAP_S)

/* Leaf 1 ebx. */
#define	CPUPT_CT_BITMAP_S	0	/* Bitmap of supported Cycle Threshold values */
#define	CPUPT_CT_BITMAP_M	(0xffff << CPUPT_CT_BITMAP_S)
#define	CPUPT_PFE_BITMAP_S	16	/* Bitmap of supported Configurable PSB Frequency encoding */
#define	CPUPT_PFE_BITMAP_M	(0xffff << CPUPT_PFE_BITMAP_S)

/*
 * Important bits in the AMD extended cpuid flags
 */
#define	AMDID_SYSCALL	0x00000800
#define	AMDID_MP		0x00080000
#define	AMDID_NX		0x00100000
#define	AMDID_EXT_MMX	0x00400000
#define	AMDID_FFXSR		0x02000000
#define	AMDID_PAGE1GB	0x04000000
#define	AMDID_RDTSCP	0x08000000
#define	AMDID_LM		0x20000000
#define	AMDID_EXT_3DNOW	0x40000000
#define	AMDID_3DNOW		0x80000000

#define	AMDID2_LAHF		0x00000001
#define	AMDID2_CMP		0x00000002
#define	AMDID2_SVM		0x00000004
#define	AMDID2_EXT_APIC	0x00000008
#define	AMDID2_CR8		0x00000010
#define	AMDID2_ABM		0x00000020
#define	AMDID2_SSE4A	0x00000040
#define	AMDID2_MAS		0x00000080
#define	AMDID2_PREFETCH	0x00000100
#define	AMDID2_OSVW		0x00000200
#define	AMDID2_IBS		0x00000400
#define	AMDID2_XOP		0x00000800
#define	AMDID2_SKINIT	0x00001000
#define	AMDID2_WDT		0x00002000
#define	AMDID2_LWP		0x00008000
#define	AMDID2_FMA4		0x00010000
#define	AMDID2_TCE		0x00020000
#define	AMDID2_NODE_ID	0x00080000
#define	AMDID2_TBM		0x00200000
#define	AMDID2_TOPOLOGY	0x00400000
#define	AMDID2_PCXC		0x00800000
#define	AMDID2_PNXC		0x01000000
#define	AMDID2_DBE		0x04000000
#define	AMDID2_PTSC		0x08000000
#define	AMDID2_PTSCEL2I	0x10000000
#define	AMDID2_MWAITX	0x20000000

/*
 * MWAIT cpu power states.  Lower 4 bits are sub-states.
 */
#define	MWAIT_C0		0xf0
#define	MWAIT_C1		0x00
#define	MWAIT_C2		0x10
#define	MWAIT_C3		0x20
#define	MWAIT_C4		0x30

/*
 * MWAIT extensions.
 */
/* Interrupt breaks MWAIT even when masked. */
#define	MWAIT_INTRBREAK		0x00000001

/*
 * AMD extended function 8000_0007h ebx info
 */
#define	AMDRAS_MCA_OF_RECOV	0x00000001
#define	AMDRAS_SUCCOR		0x00000002
#define	AMDRAS_HW_ASSERT	0x00000004
#define	AMDRAS_SCALABLE_MCA	0x00000008
#define	AMDRAS_PFEH_SUPPORT	0x00000010

/*
 * AMD extended function 8000_0007h edx info
 */
#define	AMDPM_TS			0x00000001
#define	AMDPM_FID			0x00000002
#define	AMDPM_VID			0x00000004
#define	AMDPM_TTP			0x00000008
#define	AMDPM_TM			0x00000010
#define	AMDPM_STC			0x00000020
#define	AMDPM_100MHZ_STEPS	0x00000040
#define	AMDPM_HW_PSTATE		0x00000080
#define	AMDPM_TSC_INVARIANT	0x00000100
#define	AMDPM_CPB			0x00000200

/*
 * AMD extended function 8000_0008h ebx info (amd_extended_feature_extensions)
 */
#define	AMDFEID_CLZERO			0x00000001
#define	AMDFEID_IRPERF			0x00000002
#define	AMDFEID_XSAVEERPTR		0x00000004
#define	AMDFEID_IBPB			0x00001000
#define	AMDFEID_IBRS			0x00004000
#define	AMDFEID_STIBP			0x00008000
/* The below are only defined if the corresponding base feature above exists. */
#define	AMDFEID_IBRS_ALWAYSON	0x00010000
#define	AMDFEID_STIBP_ALWAYSON	0x00020000
#define	AMDFEID_PREFER_IBRS		0x00040000
#define	AMDFEID_SSBD			0x01000000
/* SSBD via MSRC001_011F instead of MSR 0x48: */
#define	AMDFEID_VIRT_SSBD		0x02000000
#define	AMDFEID_SSB_NO			0x04000000

/*
 * AMD extended function 8000_0008h ecx info
 */
#define	AMDID_CMP_CORES			0x000000ff
#define	AMDID_COREID_SIZE		0x0000f000
#define	AMDID_COREID_SIZE_SHIFT	12

/* MSR IA32_ARCH_CAP(ABILITIES) bits */
#define	IA32_ARCH_CAP_RDCL_NO				0x00000001
#define	IA32_ARCH_CAP_IBRS_ALL				0x00000002
#define	IA32_ARCH_CAP_RSBA					0x00000004
#define	IA32_ARCH_CAP_SKIP_L1DFL_VMENTRY	0x00000008
#define	IA32_ARCH_CAP_SSB_NO				0x00000010

/*
 * Intel Processor Trace (PT) MSRs.
 */
#define	MSR_IA32_RTIT_OUTPUT_BASE		0x560	/* Trace Output Base Register (R/W) */
#define	MSR_IA32_RTIT_OUTPUT_MASK_PTRS	0x561	/* Trace Output Mask Pointers Register (R/W) */
#define	MSR_IA32_RTIT_CTL				0x570	/* Trace Control Register (R/W) */
#define	RTIT_CTL_TRACEEN				(1 << 0)
#define	RTIT_CTL_CYCEN					(1 << 1)
#define	RTIT_CTL_OS						(1 << 2)
#define	RTIT_CTL_USER					(1 << 3)
#define	RTIT_CTL_PWREVTEN				(1 << 4)
#define	RTIT_CTL_FUPONPTW				(1 << 5)
#define	RTIT_CTL_FABRICEN				(1 << 6)
#define	RTIT_CTL_CR3FILTER				(1 << 7)
#define	RTIT_CTL_TOPA					(1 << 8)
#define	RTIT_CTL_MTCEN					(1 << 9)
#define	RTIT_CTL_TSCEN					(1 << 10)
#define	RTIT_CTL_DISRETC				(1 << 11)
#define	RTIT_CTL_PTWEN					(1 << 12)
#define	RTIT_CTL_BRANCHEN				(1 << 13)
#define	RTIT_CTL_MTC_FREQ_S				14
#define	RTIT_CTL_MTC_FREQ(n)			((n) << RTIT_CTL_MTC_FREQ_S)
#define	RTIT_CTL_MTC_FREQ_M				(0xf << RTIT_CTL_MTC_FREQ_S)
#define	RTIT_CTL_CYC_THRESH_S			19
#define	RTIT_CTL_CYC_THRESH_M			(0xf << RTIT_CTL_CYC_THRESH_S)
#define	RTIT_CTL_PSB_FREQ_S				24
#define	RTIT_CTL_PSB_FREQ_M				(0xf << RTIT_CTL_PSB_FREQ_S)
#define	RTIT_CTL_ADDR_CFG_S(n) 			(32 + (n) * 4)
#define	RTIT_CTL_ADDR0_CFG_S			32
#define	RTIT_CTL_ADDR0_CFG_M			(0xfULL << RTIT_CTL_ADDR0_CFG_S)
#define	RTIT_CTL_ADDR1_CFG_S			36
#define	RTIT_CTL_ADDR1_CFG_M			(0xfULL << RTIT_CTL_ADDR1_CFG_S)
#define	RTIT_CTL_ADDR2_CFG_S			40
#define	RTIT_CTL_ADDR2_CFG_M			(0xfULL << RTIT_CTL_ADDR2_CFG_S)
#define	RTIT_CTL_ADDR3_CFG_S			44
#define	RTIT_CTL_ADDR3_CFG_M			(0xfULL << RTIT_CTL_ADDR3_CFG_S)
#define	MSR_IA32_RTIT_STATUS			0x571	/* Tracing Status Register (R/W) */
#define	RTIT_STATUS_FILTEREN			(1 << 0)
#define	RTIT_STATUS_CONTEXTEN			(1 << 1)
#define	RTIT_STATUS_TRIGGEREN			(1 << 2)
#define	RTIT_STATUS_ERROR				(1 << 4)
#define	RTIT_STATUS_STOPPED				(1 << 5)
#define	RTIT_STATUS_PACKETBYTECNT_S		32
#define	RTIT_STATUS_PACKETBYTECNT_M		(0x1ffffULL << RTIT_STATUS_PACKETBYTECNT_S)
#define	MSR_IA32_RTIT_CR3_MATCH			0x572	/* Trace Filter CR3 Match Register (R/W) */
#define	MSR_IA32_RTIT_ADDR_A(n)			(0x580 + (n) * 2)
#define	MSR_IA32_RTIT_ADDR_B(n)			(0x581 + (n) * 2)
#define	MSR_IA32_RTIT_ADDR0_A			0x580	/* Region 0 Start Address (R/W) */
#define	MSR_IA32_RTIT_ADDR0_B			0x581	/* Region 0 End Address (R/W) */
#define	MSR_IA32_RTIT_ADDR1_A			0x582	/* Region 1 Start Address (R/W) */
#define	MSR_IA32_RTIT_ADDR1_B			0x583	/* Region 1 End Address (R/W) */
#define	MSR_IA32_RTIT_ADDR2_A			0x584	/* Region 2 Start Address (R/W) */
#define	MSR_IA32_RTIT_ADDR2_B			0x585	/* Region 2 End Address (R/W) */
#define	MSR_IA32_RTIT_ADDR3_A			0x586	/* Region 3 Start Address (R/W) */
#define	MSR_IA32_RTIT_ADDR3_B			0x587	/* Region 3 End Address (R/W) */

/* Intel Processor Trace Table of Physical Addresses (ToPA). */
#define	TOPA_SIZE_S			6
#define	TOPA_SIZE_M			(0xf << TOPA_SIZE_S)
#define	TOPA_SIZE_4K		(0 << TOPA_SIZE_S)
#define	TOPA_SIZE_8K		(1 << TOPA_SIZE_S)
#define	TOPA_SIZE_16K		(2 << TOPA_SIZE_S)
#define	TOPA_SIZE_32K		(3 << TOPA_SIZE_S)
#define	TOPA_SIZE_64K		(4 << TOPA_SIZE_S)
#define	TOPA_SIZE_128K		(5 << TOPA_SIZE_S)
#define	TOPA_SIZE_256K		(6 << TOPA_SIZE_S)
#define	TOPA_SIZE_512K		(7 << TOPA_SIZE_S)
#define	TOPA_SIZE_1M		(8 << TOPA_SIZE_S)
#define	TOPA_SIZE_2M		(9 << TOPA_SIZE_S)
#define	TOPA_SIZE_4M		(10 << TOPA_SIZE_S)
#define	TOPA_SIZE_8M		(11 << TOPA_SIZE_S)
#define	TOPA_SIZE_16M		(12 << TOPA_SIZE_S)
#define	TOPA_SIZE_32M		(13 << TOPA_SIZE_S)
#define	TOPA_SIZE_64M		(14 << TOPA_SIZE_S)
#define	TOPA_SIZE_128M		(15 << TOPA_SIZE_S)
#define	TOPA_STOP			(1 << 4)
#define	TOPA_INT			(1 << 2)
#define	TOPA_END			(1 << 0)

/*
 * Constants related to MSR's.
 */
#define	APICBASE_RESERVED	0x000002ff
#define	APICBASE_BSP		0x00000100
#define	APICBASE_X2APIC		0x00000400
#define	APICBASE_ENABLED	0x00000800
#define	APICBASE_ADDRESS	0xfffff000

/* MSR_IA32_FEATURE_CONTROL related */
#define	IA32_FEATURE_CONTROL_LOCK	0x01	/* lock bit */
#define	IA32_FEATURE_CONTROL_SMX_EN	0x02	/* enable VMX inside SMX */
#define	IA32_FEATURE_CONTROL_VMX_EN	0x04	/* enable VMX outside SMX */

/* MSR IA32_MISC_ENABLE */
#define	IA32_MISC_EN_FASTSTR		0x0000000000000001ULL
#define	IA32_MISC_EN_ATCCE			0x0000000000000008ULL
#define	IA32_MISC_EN_PERFMON		0x0000000000000080ULL
#define	IA32_MISC_EN_PEBSU			0x0000000000001000ULL
#define	IA32_MISC_EN_ESSTE			0x0000000000010000ULL
#define	IA32_MISC_EN_MONE			0x0000000000040000ULL
#define	IA32_MISC_EN_LIMCPUID		0x0000000000400000ULL
#define	IA32_MISC_EN_xTPRD			0x0000000000800000ULL
#define	IA32_MISC_EN_XDD			0x0000000400000000ULL

/*
 * IA32_SPEC_CTRL and IA32_PRED_CMD MSRs are described in the Intel'
 * document 336996-001 Speculative Execution Side Channel Mitigations.
 *
 * AMD uses the same MSRs and bit definitions, as described in 111006-B
 * "Indirect Branch Control Extension" and 124441 "Speculative Store Bypass
 * Disable."
 */
/* MSR IA32_SPEC_CTRL */
#define	IA32_SPEC_CTRL_IBRS			0x00000001
#define	IA32_SPEC_CTRL_STIBP		0x00000002
#define	IA32_SPEC_CTRL_SSBD			0x00000004

/* MSR IA32_PRED_CMD */
#define	IA32_PRED_CMD_IBPB_BARRIER	0x0000000000000001ULL

/* MSR IA32_FLUSH_CMD */
#define	IA32_FLUSH_CMD_L1D			0x00000001

/* MSR IA32_HWP_CAPABILITIES */
#define	IA32_HWP_CAPABILITIES_HIGHEST_PERFORMANCE(x)	(((x) >> 0) & 0xff)
#define	IA32_HWP_CAPABILITIES_GUARANTEED_PERFORMANCE(x)	(((x) >> 8) & 0xff)
#define	IA32_HWP_CAPABILITIES_EFFICIENT_PERFORMANCE(x)	(((x) >> 16) & 0xff)
#define	IA32_HWP_CAPABILITIES_LOWEST_PERFORMANCE(x)		(((x) >> 24) & 0xff)

/* MSR IA32_HWP_REQUEST */
#define	IA32_HWP_REQUEST_MINIMUM_VALID					(1ULL << 63)
#define	IA32_HWP_REQUEST_MAXIMUM_VALID					(1ULL << 62)
#define	IA32_HWP_REQUEST_DESIRED_VALID					(1ULL << 61)
#define	IA32_HWP_REQUEST_EPP_VALID 						(1ULL << 60)
#define	IA32_HWP_REQUEST_ACTIVITY_WINDOW_VALID			(1ULL << 59)
#define	IA32_HWP_REQUEST_PACKAGE_CONTROL				(1ULL << 42)
#define	IA32_HWP_ACTIVITY_WINDOW						(0x3ffULL << 32)
#define	IA32_HWP_REQUEST_ENERGY_PERFORMANCE_PREFERENCE	(0xffULL << 24)
#define	IA32_HWP_DESIRED_PERFORMANCE					(0xffULL << 16)
#define	IA32_HWP_REQUEST_MAXIMUM_PERFORMANCE			(0xffULL << 8)
#define	IA32_HWP_MINIMUM_PERFORMANCE					(0xffULL << 0)


/* AMD Write Allocate Top-Of-Memory and Control Register */
#define	AMD_WT_ALLOC_TME			0x40000	/* top-of-memory enable */
#define	AMD_WT_ALLOC_PRE			0x20000	/* programmable range enable */
#define	AMD_WT_ALLOC_FRE			0x10000	/* fixed (A0000-FFFFF) range enable */

#endif /* _MACHINE_CPUID_H_ */
