/*-
 * Copyright (c) 1995 Bruce D. Evans.
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
 * 3. Neither the name of the author nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _I386_CPUVAR_H_
#define	_I386_CPUVAR_H_

/*
 * Miscellaneous machine-dependent declarations.
 */

extern	long		Maxmem;
extern	u_int		basemem;
extern	int			busdma_swi_pending;
extern	u_int		cpu_exthigh;
extern	u_int		cpu_feature;
extern	u_int		cpu_feature2;
extern	u_int		amd_feature;
extern	u_int		amd_feature2;
extern	u_int		amd_rascap;
extern	u_int		amd_pminfo;
extern	u_int		amd_extended_feature_extensions;
extern	u_int		via_feature_rng;
extern	u_int		via_feature_xcrypt;
extern	u_int		cpu_clflush_line_size;
extern	u_int		cpu_stdext_feature;
extern	u_int		cpu_stdext_feature2;
extern	u_int		cpu_stdext_feature3;
extern	uint64_t	cpu_ia32_arch_caps;
extern	u_int		cpu_fxsr;
extern	u_int		cpu_high;
extern	u_int		cpu_id;
extern	u_int		cpu_max_ext_state_size;
extern	u_int		cpu_mxcsr_mask;
extern	u_int		cpu_procinfo;
extern	u_int		cpu_procinfo2;
extern	char		cpu_vendor[];
extern	u_int		cpu_vendor_id;
extern	u_int		cpu_mon_mwait_flags;
extern	u_int		cpu_mon_min_size;
extern	u_int		cpu_mon_max_size;
extern	u_int		cpu_maxphyaddr;
extern	u_int		cpu_power_eax;
extern	u_int		cpu_power_ebx;
extern	u_int		cpu_power_ecx;
extern	u_int		cpu_power_edx;

extern	u_int		max_apic_id;

void	finishidentcpu(void);
bool	fix_cpuid(void);
void	identify_cpu1(void);
void	identify_cpu2(void);
void	identify_cpu_fixup_bsp(void);
void	initializecpu(void);
void	initializecpucache(void);
void	printcpuinfo(void);
int		pti_get_default(void);

#define	MSR_OP_ANDNOT		0x00000001
#define	MSR_OP_OR			0x00000002
#define	MSR_OP_WRITE		0x00000003
#define	MSR_OP_LOCAL		0x10000000
#define	MSR_OP_SCHED		0x20000000
#define	MSR_OP_RENDEZVOUS	0x30000000

#endif
