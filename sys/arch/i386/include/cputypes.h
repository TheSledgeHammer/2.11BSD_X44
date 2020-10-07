/*	$NetBSD: cputypes.h,v 1.10.2.1 1998/10/16 17:39:32 cgd Exp $	*/

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * Classes of processor.
 */
#define	CPUCLASS_286		0
#define	CPUCLASS_386		1
#define	CPUCLASS_486		2
#define	CPUCLASS_586		3
#define	CPUCLASS_686		4

/*
 * Kinds of processor
 */
#define	CPU_286				0	/* Intel 80286 */
#define	CPU_386SX			1	/* Intel 80386SX */
#define	CPU_386				2	/* Intel 80386DX */
#define	CPU_486SX			3	/* Intel 80486SX */
#define	CPU_486				4	/* Intel 80486DX */
#define	CPU_586				5	/* Intel Pentium */
#define	CPU_686				6	/* Pentium Pro */
#define	CPU_PII				7	/* Intel Pentium II */
#define	CPU_PIII			8	/* Intel Pentium III */
#define	CPU_P4				9	/* Intel Pentium 4 */

#define	CPU_486DLC			10	/* Cyrix 486DLC */
#define	CPU_M1SC			11	/* Cyrix M1sc (aka 5x86) */
#define	CPU_M1				12	/* Cyrix M1 (aka 6x86) */
#define	CPU_BLUE			13	/* IBM BlueLighting CPU */
#define	CPU_M2				14	/* Cyrix M2 (aka enhanced 6x86 with MMX */
#define	CPU_NX586			15	/* NexGen (now AMD) 586 */
#define	CPU_CY486DX			16	/* Cyrix 486S/DX/DX2/DX4 */

#define CPU_AM586			17	/* AMD Am486 and Am5x86 */
#define CPU_K5				18	/* AMD K5 */
#define CPU_K6				19	/* NexGen 686 aka AMD K6 */
#define CPU_C6				20	/* IDT WinChip C6 */
#define CPU_TMX86			21	/* Transmeta TMx86 */
#define	CPU_GEODE1100		22	/* NS Geode SC1100 */

/*
 * CPU vendors
 */
#define CPUVENDOR_UNKNOWN	0
#define CPUVENDOR_INTEL		1				/* Intel */
#define CPUVENDOR_CYRIX		2				/* Cyrix */
#define CPUVENDOR_NEXGEN	3				/* Nexgen */
#define CPUVENDOR_AMD		4				/* AMD */
#define CPUVENDOR_IDT		5				/* Centaur/IDT/VIA */
#define CPUVENDOR_TRANSMETA	6				/* Transmeta */
#define	CPUVENDOR_IBM		7				/* IBM */
#define	CPUVENDOR_RISE		8				/* Rise */
#define	CPUVENDOR_CENTAUR	CPUVENDOR_IDT
#define	CPUVENDOR_HYGON		9				/* Hygon */
#define	CPUVENDOR_NSC		10				/* NSC */
#define	CPUVENDOR_SIS		12				/* SiS */
#define	CPUVENDOR_UMC		13				/* UMC */

/*
 * Some other defines, dealing with values returned by cpuid.
 */
#define CPU_MAXMODEL		15	/* Models within family range 0-15 */
#define CPU_DEFMODEL		16	/* Value for unknown model -> default  */
#define CPU_MINFAMILY		 4	/* Lowest that cpuid can return (486) */
#define CPU_MAXFAMILY		 6	/* Highest we know (686) */
