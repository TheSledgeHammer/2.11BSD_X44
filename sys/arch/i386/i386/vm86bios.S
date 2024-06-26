/*-
 * Copyright (c) 1998 Jonathan Lemon
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

#include "assym.h"

#include <machine/asm.h>				/* miscellaneous asm macros */
#include <machine/segments.h>
#include <machine/trap.h>


#define SCR_NEWPTD		PCB_ESI			/* readability macros */
#define SCR_VMFRAME		PCB_EBP			/* see vm86.c for explanation */
#define SCR_STACK		PCB_ESP
#define SCR_PGTABLE		PCB_EBX
#define SCR_ARGFRAME	PCB_EIP
#define SCR_TSS0		PCB_VM86
#define SCR_TSS1		(PCB_VM86+4)

/*
 * FreeBSD: see FreeBSD's i386/asmacros.h
 * Access per-CPU data. 
 */
#define	PERCPU(member)	%fs:PC_ ## member
#define	PERCPU_ADDR(member, reg)					\
		movl 	%fs:PC_PRVSPACE, reg 				;\
		addl 	$PC_ ## member, reg


		.data
		ALIGN_DATA

		.globl	vm86pcb

vm86pcb:		.long	0

		.text

/*
 * vm86_bioscall(struct trapframe_vm86 *vm86)
 */
ENTRY(vm86_bioscall)
		movl	vm86pcb,%edx			/* scratch data area */
		movl	4(%esp),%eax
		movl	%eax,SCR_ARGFRAME(%edx)	/* save argument pointer */
		pushl	%ebx
		pushl	%ebp
		pushl	%esi
		pushl	%edi
		pushl	%gs

		movl	CPUVAR(CURPROC),%ecx		/* todo: fix */
		cmpl	%ecx,CPUVAR(CURPROC)		/* do we need to save fp? */
		jne		1f
		pushl	%edx
		movl	U_PCB(%ecx),%ecx
		pushl	PCB_SAVEFPU(%ecx)
		movl	_C_LABEL(npxsave),%eax
		call	*%eax
		addl	$4,%esp
		popl	%edx					/* recover our pcb */
1:
		movl	SCR_VMFRAME(%edx),%ebx	/* target frame location */
		movl	%ebx,%edi				/* destination */
		movl    SCR_ARGFRAME(%edx),%esi	/* source (set on entry) */
		movl	$VM86_FRAMESIZE/4,%ecx	/* sizeof(struct vm86frame)/4 */
		cld
		rep
		movsl							/* copy frame to new stack */

		movl	_C_LABEL(curpcb),%eax
		pushl	%eax					/* save curpcb */
		movl	%edx,_C_LABEL(curpcb)	/* set curpcb to vm86pcb */

		movl	PERCPU(TSS_GDT),%ebx	/* entry in GDT */
		movl	0(%ebx),%eax
		movl	%eax,SCR_TSS0(%edx)		/* save first word */
		movl	4(%ebx),%eax
		andl    $~0x200, %eax			/* flip 386BSY -> 386TSS */
		movl	%eax,SCR_TSS1(%edx)		/* save second word */

		movl	PCB_TSSD(%edx),%edi		/* vm86 tssd entry */
		movl	0(%edi),%eax
		movl	%eax,0(%ebx)
		movl	4(%edi),%eax
		movl	%eax,4(%ebx)
		movl	$GPROC0_SEL*8,%esi		/* GSEL(entry, SEL_KPL) */
		ltr		%si

		movl	%cr3,%eax
		pushl	%eax					/* save address space */
		movl	_C_LABEL(IdlePTD),%ecx	/* va (and pa) of Idle PTD */
		movl	%ecx,%ebx
		movl	0(%ebx),%eax
		pushl	%eax					/* old ptde != 0 when booting */
		pushl	%ebx					/* keep for reuse */

		movl	%esp,SCR_STACK(%edx)	/* save current stack location */

		movl	SCR_NEWPTD(%edx),%eax	/* mapping for vm86 page table */
		movl	%eax,0(%ebx)			/* ... install as PTD entry 0 */

#if defined(PMAP_PAE_COMP)
		movl	_C_LABEL(IdlePDPT),%ecx
#endif
		movl	%ecx,%cr3				/* new page tables */
		movl	SCR_VMFRAME(%edx),%esp	/* switch to new stack */

		pushl	%esp
		movl	_C_LABEL(vm86_prepcall), %eax
		call	*%eax					/* finish setup */
		add		$4, %esp

		/*
		 * Return via doreti
		 */
		jmp		_C_LABEL(Xdoreti)


/*
 * vm86_biosret(struct trapframe_vm86 *vm86)
 */
ENTRY(vm86_biosret)
		movl	vm86pcb,%edx			/* data area */

		movl	4(%esp),%esi			/* source */
		movl	SCR_ARGFRAME(%edx),%edi	/* destination */
		movl	$VM86_FRAMESIZE/4,%ecx	/* size */
		cld
		rep
		movsl							/* copy frame to original frame */

		movl	SCR_STACK(%edx),%esp	/* back to old stack */
		popl	%ebx					/* saved va of Idle PTD */
		popl	%eax
		movl	%eax,0(%ebx)			/* restore old pte */
		popl	%eax
		movl	%eax,%cr3				/* install old page table */

		movl	PERCPU(TSS_GDT),%ebx	/* entry in GDT */
		movl	SCR_TSS0(%edx),%eax
		movl	%eax,0(%ebx)			/* restore first word */
		movl	SCR_TSS1(%edx),%eax
		movl	%eax,4(%ebx)			/* restore second word */
		movl	$GPROC0_SEL*8,%esi		/* GSEL(entry, SEL_KPL) */
		ltr		%si

		popl	_C_LABEL(curpcb)		/* restore curpcb/curproc */
		movl	SCR_ARGFRAME(%edx),%edx	/* original stack frame */
		movl	TF_TRAPNO(%edx),%eax	/* return (trapno) */

		popl	%gs
		popl	%edi
		popl	%esi
		popl	%ebp
		popl	%ebx
		ret								/* back to our normal program */
