/*-
 * Copyright (c) 1989, 1990 William F. Jolitz
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
 *	@(#)segments.h	8.1 (Berkeley) 6/11/93
 */

/*
 * 386 Segmentation Data Structures and definitions
 *	William F. Jolitz (william@ernie.berkeley.edu) 6/20/1989
 */

#ifndef _I386_SEGMENTS_H_
#define _I386_SEGMENTS_H_

/*
 * Selectors
 */
#define	ISPL(s)		((s)&3)					/* what is the priority level of a selector */
#define	SEL_KPL		0						/* kernel priority level */
#define	SEL_UPL		3						/* user priority level */
#define	ISLDT(s)	((s)&SEL_LDT)			/* is it local or global */
#define	SEL_LDT		4						/* local descriptor table */
#define	IDXSEL(s)	(((s)>>3) & 0x1fff)		/* index of selector */

#define	LSEL(s,r)	(((s)<<3) | SEL_LDT | r)/* a local selector */
#define	GSEL(s,r)	(((s)<<3) | r)			/* a global selector */


#define	USERMODE(c, f)	(ISPL(c) == SEL_UPL)
#define	KERNELMODE(c, f)(ISPL(c) == SEL_KPL)

/*
 * Memory and System segment descriptors
 */
struct	segment_descriptor	{
	unsigned sd_lolimit:16 ;	/* segment extent (lsb) */
	unsigned sd_lobase:24 ;		/* segment base address (lsb) */
	unsigned sd_type:5 ;		/* segment type */
	unsigned sd_dpl:2 ;		/* segment descriptor priority level */
	unsigned sd_p:1 ;		/* segment descriptor present */
	unsigned sd_hilimit:4 ;		/* segment extent (msb) */
	unsigned sd_xx:2 ;		/* unused */
	unsigned sd_def32:1 ;		/* default 32 vs 16 bit size */
	unsigned sd_gran:1 ;		/* limit granularity (byte/page units)*/
	unsigned sd_hibase:8 ;		/* segment base address  (msb) */
} ;

/*
 * Gate descriptors (e.g. indirect descriptors)
 */
struct	gate_descriptor	{
	unsigned gd_looffset:16 ;	/* gate offset (lsb) */
	unsigned gd_selector:16 ;	/* gate segment selector */
	unsigned gd_stkcpy:5 ;		/* number of stack wds to cpy */
	unsigned gd_xx:3 ;		/* unused */
	unsigned gd_type:5 ;		/* segment type */
	unsigned gd_dpl:2 ;		/* segment descriptor priority level */
	unsigned gd_p:1 ;		/* segment descriptor present */
	unsigned gd_hioffset:16 ;	/* gate offset (msb) */
} ;

/*
 * Generic descriptor
 */
union descriptor	{
	struct	segment_descriptor 	sd;
	struct	gate_descriptor 	gd;
};

/* system segments and gate types */
#define	SDT_SYSNULL	 	 0	/* system null */
#define	SDT_SYS286TSS	 1	/* system 286 TSS available */
#define	SDT_SYSLDT	 	 2	/* system local descriptor table */
#define	SDT_SYS286BSY	 3	/* system 286 TSS busy */
#define	SDT_SYS286CGT	 4	/* system 286 call gate */
#define	SDT_SYSTASKGT	 5	/* system task gate */
#define	SDT_SYS286IGT	 6	/* system 286 interrupt gate */
#define	SDT_SYS286TGT	 7	/* system 286 trap gate */
#define	SDT_SYSNULL2	 8	/* system null again */
#define	SDT_SYS386TSS	 9	/* system 386 TSS available */
#define	SDT_SYSNULL3	10	/* system null again */
#define	SDT_SYS386BSY	11	/* system 386 TSS busy */
#define	SDT_SYS386CGT	12	/* system 386 call gate */
#define	SDT_SYSNULL4	13	/* system null again */
#define	SDT_SYS386IGT	14	/* system 386 interrupt gate */
#define	SDT_SYS386TGT	15	/* system 386 trap gate */

	/* memory segment types */
#define	SDT_MEMRO		16	/* memory read only */
#define	SDT_MEMROA		17	/* memory read only accessed */
#define	SDT_MEMRW		18	/* memory read write */
#define	SDT_MEMRWA		19	/* memory read write accessed */
#define	SDT_MEMROD		20	/* memory read only expand dwn limit */
#define	SDT_MEMRODA		21	/* memory read only expand dwn limit accessed */
#define	SDT_MEMRWD		22	/* memory read write expand dwn limit */
#define	SDT_MEMRWDA		23	/* memory read write expand dwn limit acessed */
#define	SDT_MEME		24	/* memory execute only */
#define	SDT_MEMEA		25	/* memory execute only accessed */
#define	SDT_MEMER		26	/* memory execute read */
#define	SDT_MEMERA		27	/* memory execute read accessed */
#define	SDT_MEMEC		28	/* memory execute only conforming */
#define	SDT_MEMEAC		29	/* memory execute only accessed conforming */
#define	SDT_MEMERC		30	/* memory execute read conforming */
#define	SDT_MEMERAC		31	/* memory execute read accessed conforming */


/* is memory segment descriptor pointer ? */
#define ISMEMSDP(s)	((s->d_type) >= SDT_MEMRO && (s->d_type) <= SDT_MEMERAC)

/* is 286 gate descriptor pointer ? */
#define IS286GDP(s)	(((s->d_type) >= SDT_SYS286CGT \
				 && (s->d_type) < SDT_SYS286TGT))

/* is 386 gate descriptor pointer ? */
#define IS386GDP(s)	(((s->d_type) >= SDT_SYS386CGT \
				&& (s->d_type) < SDT_SYS386TGT))

/* is gate descriptor pointer ? */
#define ISGDP(s)	(IS286GDP(s) || IS386GDP(s))

/* is segment descriptor pointer ? */
#define ISSDP(s)	(ISMEMSDP(s) || !ISGDP(s))

/* is system segment descriptor pointer ? */
#define ISSYSSDP(s)	(!ISMEMSDP(s) && !ISGDP(s))

/*
 * Software definitions are in this convenient format,
 * which are translated into inconvenient segment descriptors
 * when needed to be used by the 386 hardware
 */

struct soft_segment_descriptor	{
	unsigned ssd_base ;			/* segment base address  */
	unsigned ssd_limit ;		/* segment extent */
	unsigned ssd_type:5 ;		/* segment type */
	unsigned ssd_dpl:2 ;		/* segment descriptor priority level */
	unsigned ssd_p:1 ;			/* segment descriptor present */
	unsigned ssd_xx:4 ;			/* unused */
	unsigned ssd_xx1:2 ;		/* unused */
	unsigned ssd_def32:1 ;		/* default 32 vs 16 bit size */
	unsigned ssd_gran:1 ;		/* limit granularity (byte/page units)*/
};

/*
 * region descriptors, used to load gdt/idt tables before segments yet exist
 */
struct region_descriptor {
	unsigned rd_limit:16 ;		/* segment extent */
	unsigned rd_base:32;		/* base address  */
};

/* global descriptor table */
#define	GNULL_SEL		0	/* Null Descriptor */
#define GUFS_SEL		1	/* %fs Descriptor */
#define GUGS_SEL		2	/* %gs Descriptor */
#define	GCODE_SEL		3	/* Kernel Code Descriptor */
#define	GDATA_SEL		4	/* Kernel Data Descriptor */
#define GUCODE_SEL		5	/* User Code Descriptor */
#define GUDATA_SEL		6	/* User Data Descriptor */
#define	GPROC0_SEL		7	/* Task state process slot zero and up */
#define	GLDT_SEL		8	/* LDT - eventually one per process */
#define GUSERLDT_SEL		9	/* User LDT Descriptor */
#define	GPANIC_SEL		10	/* Task state to consider panic from */
#define	GTGATE_SEL		11	/* Process task switch gate */
#define	GINVTSS_SEL		12	/* Task state to take invalid tss on */
#define	GDBLFLT_SEL		13	/* Task state to take double fault on */
#define	GEXIT_SEL		14	/* Task state to process cpu_texit() on */
#define GBIOSCODE32_SEL 	15	/* BIOS interface (32bit Code) */
#define GBIOSCODE16_SEL 	16	/* BIOS interface (16bit Code) */
#define GBIOSDATA_SEL		17	/* BIOS interface (Data) */
#define GBIOSUTIL_SEL		18	/* BIOS interface (Utility) */
#define GBIOSARGS_SEL 		19	/* BIOS interface (Arguments) */

#define NGDT 			20

/* local descriptor table */
#define	LUNULL_SEL		0	/* Null Descriptor */
#define	LSYS5CALLS_SEL		1	/* forced by intel BCS */
#define	LSYS5SIGR_SEL		2
#define	LUCODE_SEL		3
#define	LUDATA_SEL		4
#define	L43BSDCALLS_SEL		5	/* notyet */

#define NLDT			7

/*
 * Segment Protection Exception code bits
 */

#define	SEGEX_EXT		0x01	/* recursive or externally induced */
#define	SEGEX_IDT		0x02	/* interrupt descriptor table */
#define	SEGEX_TI		0x04	/* local descriptor table */
					/* ther bits are affected descriptor index */
#define SEGEX_IDX(s)		((s)>>3)&0x1fff)

/*
 * Entries in the Interrupt Descriptor Table (IDT)
 */
#define	NIDT			256
#define	NRSVIDT			32	/* reserved entries for cpu exceptions */

/*
 * Entries in the Interrupt Descriptor Table (IDT)
 */
#define	IDT_DE			0	/* #DE: Divide Error */
#define	IDT_DB			1	/* #DB: Debug */
#define	IDT_NMI			2	/* Nonmaskable External Interrupt */
#define	IDT_BP			3	/* #BP: Breakpoint */
#define	IDT_OF			4	/* #OF: Overflow */
#define	IDT_BR			5	/* #BR: Bound Range Exceeded */
#define	IDT_UD			6	/* #UD: Undefined/Invalid Opcode */
#define	IDT_NM			7	/* #NM: No Math Coprocessor */
#define	IDT_DF			8	/* #DF: Double Fault */
#define	IDT_FPUGP		9	/* Coprocessor Segment Overrun */
#define	IDT_TS			10	/* #TS: Invalid TSS */
#define	IDT_NP			11	/* #NP: Segment Not Present */
#define	IDT_SS			12	/* #SS: Stack Segment Fault */
#define	IDT_GP			13	/* #GP: General Protection Fault */
#define	IDT_PF			14	/* #PF: Page Fault */
#define	IDT_MF			16	/* #MF: FPU Floating-Point Error */
#define	IDT_AC			17	/* #AC: Alignment Check */
#define	IDT_MC			18	/* #MC: Machine Check */
#define	IDT_XF			19	/* #XF: SIMD Floating-Point Exception */
#define	IDT_IO_INTS		NRSVIDT	/* Base of IDT entries for I/O interrupts. */
#define	IDT_SYSCALL		0x80	/* System Call Interrupt Vector */
//#define	IDT_DTRACE_RET		0x92	/* DTrace pid provider Interrupt Vector */
//#define	IDT_EVTCHN		0x93	/* Xen HVM Event Channel Interrupt Vector */

#ifdef _KERNEL
extern struct soft_segment_descriptor *gdt_segs;
extern struct soft_segment_descriptor *ldt_segs;

void 	ssdtosd(struct soft_segment_descriptor *ssd, struct segment_descriptor *sd) ;	/* to decode a ssd */
void 	sdtossd(struct segment_descriptor *sd, struct soft_segment_descriptor *ssd) ;	/* to encode a sd */
void	lgdt(struct region_descriptor *rdp);
#endif
#endif /* _I386_SEGMENTS_H_ */
