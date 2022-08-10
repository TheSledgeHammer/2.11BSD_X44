/*
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
 *	from: @(#)locore.s	7.3 (Berkeley) 5/13/91
 *	from NetBSD: Id: locore.s,v 1.12 1993/05/27 16:44:13 cgd Exp
 * $FreeBSD$
 *      @(#)locore.s	8.3 (Berkeley) 9/23/93
 */

#include "assym.h"

#include <sys/errno.h>

#include <machine/asm.h>
//#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/trap.h>

/**********************************************************************/
/* Scheduling */
/*
 * The following primitives manipulate the run queues.  _whichqs tells which
 * of the 32 queues _qs have processes in them.  Setrunqueue puts processes
 * into queues, Remrq removes them from queues.  The running process is on
 * no queue, other processes are on a queue related to p->p_priority, divided
 * by 4 actually to shrink the 0-127 range of priorities into the 32 available
 * queues.
 */
		.globl	_C_LABEL(whichqs),_C_LABEL(qs),_C_LABEL(cnt),_C_LABEL(panic)
		.comm	_C_LABEL(noproc),4
		.comm	_C_LABEL(runrun),4

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
ENTRY(idle)
		call	_C_LABEL(spl0)
		cmpl	$0,_C_LABEL(whichqs)
		jne		sw1
		hlt						# wait for interrupt
		jmp		idle

		.align 	4 					/* ..so that profiling doesn't lump Idle with Xswitch().. */
badsw:
		pushl	$sw0
		call	_C_LABEL(panic)
		/*NOTREACHED*/

/*
 * Swtch()
 */
ENTRY(cpu_switch)

		incl	_C_LABEL(cnt)+V_SWTCH

/* switch to new process. first, save context as needed */

		movl	_C_LABEL(curproc),%ecx

/* if no process to save, don't bother */
		cmpl	$0,%ecx
		je		sw1

		movl	P_ADDR(%ecx),%ecx


		movl	(%esp),%eax							# Hardware registers
		movl	%eax, PCB_EIP(%ecx)
		movl	%ebx, PCB_EBX(%ecx)
		movl	%esp, PCB_ESP(%ecx)
		movl	%ebp, PCB_EBP(%ecx)
		movl	%esi, PCB_ESI(%ecx)
		movl	%edi, PCB_EDI(%ecx)

#ifdef NPX
/* have we used fp, and need a save? */
		mov		_C_LABEL(curproc),%eax
		cmp		%eax,_C_LABEL(npxproc)
		jne		1f
		pushl	%ecx								/* h/w bugs make saving complicated */
		leal	PCB_SAVEFPU(%ecx),%eax
		pushl	%eax
		call	_C_LABEL(npxsave)					/* do it in a big C function */
		popl	%eax
		popl	%ecx
1:
#endif

		movl	_C_LABEL(CMAP2),%eax							# save temporary map PTE
		movl	%eax,PCB_CMAP2(%ecx)				# in our context
		movl	$0,_C_LABEL(curproc)				# out of process

		# movw	_C_LABEL(cpl), %ax
		# movw	%ax, PCB_IML(%ecx)	# save ipl

/* save is done, now choose a new process or idle */
sw1:
		movl	_C_LABEL(whichqs),%edi
2:
		cli
		bsfl	%edi,%eax							# find a full q
		jz		idle								# if none, idle
# XX update whichqs?
swfnd:
		btrl	%eax,%edi							# clear q full status
		jnb		2b									# if it was clear, look for another
		movl	%eax,%ebx							# save which one we are using

		shll	$3,%eax
		addl	$_C_LABEL(qs),%eax					# select q
		movl	%eax,%esi

#ifdef	DIAGNOSTIC
		cmpl	P_FORW(%eax),%eax 					# linked to self? (e.g. not on list)
		je		badsw								# not possible
#endif

		movl	P_FORW(%eax),%ecx					# unlink from front of process q
		movl	P_FORW(%ecx),%edx
		movl	%edx,P_FORW(%eax)
		movl	P_BACK(%ecx),%eax
		movl	%eax,P_BACK(%edx)

		cmpl	P_FORW(%ecx),%esi					# q empty
		je		3f
		btsl	%ebx,%edi							# nope, set to indicate full
3:
		movl	%edi,_C_LABEL(whichqs)				# update q status

		movl	$0,%eax
		call	_C_LABEL(want_resched)

#ifdef	DIAGNOSTIC
		cmpl	%eax,P_WCHAN(%ecx)
		jne		badsw
		cmpb	$ SRUN,P_STAT(%ecx)
		jne		badsw
#endif

		movl	%eax,P_BACK(%ecx)					/* isolate process to run */
		movl	P_ADDR(%ecx),%edx
		movl	PCB_CR3(%edx),%ebx

		/* switch address space */
		movl	%ebx,%cr3

		/* restore context */
		movl	PCB_EBX(%edx), %ebx
		movl	PCB_ESP(%edx), %esp
		movl	PCB_EBP(%edx), %ebp
		movl	PCB_ESI(%edx), %esi
		movl	PCB_EDI(%edx), %edi
		movl	PCB_EIP(%edx), %eax
		movl	%eax, (%esp)

		movl	PCB_CMAP2(%edx),%eax				# get temporary map
		movl	%eax,CMAP2							# reload temporary map PTE

		movl	%ecx,_C_LABEL(curproc)				# into next process
		movl	%edx,_C_LABEL(curpcb)

		/* pushl	PCB_IML(%edx)
		call	_splx
		popl	%eax*/

		movl	%edx,%eax							# return (1);
		ret

ENTRY(mvesp)
		movl	%esp,%eax
		ret

ENTRY(proc_trampoline)
		movl	$IPL_NONE,_C_LABEL(cpl)
		pushl	%ebx
		call	*%esi
		addl	$4,%esp
		INTRFASTEXIT

/*
 * savectx(pcb, altreturn)
 * Update pcb, saving current processor state and arranging
 * for alternate return ala longjmp in Xswitch if altreturn is true.
 */
ENTRY(savectx)
		movl	4(%esp), %ecx
		movw	_C_LABEL(cpl), %ax
		movw	%ax, PCB_IML(%ecx)
		movl	(%esp), %eax
		movl	%eax, PCB_EIP(%ecx)
		movl	%ebx, PCB_EBX(%ecx)
		movl	%esp, PCB_ESP(%ecx)
		movl	%ebp, PCB_EBP(%ecx)
		movl	%esi, PCB_ESI(%ecx)
		movl	%edi, PCB_EDI(%ecx)

#ifdef NPX
/*
 * If npxproc == NULL, then the npx h/w state is irrelevant and the
 * state had better already be in the pcb.  This is true for forks
 * but not for dumps (the old book-keeping with FP flags in the pcb
 * always lost for dumps because the dump pcb has 0 flags).
 *
 * If npxproc != NULL, then we have to save the npx h/w state to
 * npxproc's pcb and copy it to the requested pcb, or save to the
 * requested pcb and reload.  Copying is easier because we would
 * have to handle h/w bugs for reloading.  We used to lose the
 * parent's npx state for forks by forgetting to reload.
 */
		mov		_C_LABEL(npxproc),%eax
		testl	%eax,%eax
  		je		1f

		pushl	%ecx
		movl	P_ADDR(%eax),%eax
		leal	PCB_SAVEFPU(%eax),%eax
		pushl	%eax
		pushl	%eax
		call	_C_LABEL(npxsave)
		popl	%eax
		popl	%eax
		popl	%ecx

		pushl	%ecx
		pushl	$108+8*2						/* XXX h/w state size + padding */
		leal	PCB_SAVEFPU(%ecx),%ecx
		pushl	%ecx
		pushl	%eax
		call	_C_LABEL(bcopy)
		addl	$12,%esp
		popl	%ecx
1:
#endif

		movl	_C_LABEL(CMAP2), %edx					# save temporary map PTE
		movl	%edx, PCB_CMAP2(%ecx)			# in our context

		cmpl	$0, 8(%esp)
		je		1f
		movl	%esp, %edx						# relocate current sp relative to pcb
		subl	$_C_LABEL(proc0kstack), %edx	# (sp is relative to kstack):
		addl	%edx, %ecx						# pcb += sp - kstack;
		movl	%eax, (%ecx)					# write return pc at (relocated) sp@
		# this mess deals with replicating register state gcc hides
		movl	12(%esp),%eax
		movl	%eax,12(%ecx)
		movl	16(%esp),%eax
		movl	%eax,16(%ecx)
		movl	20(%esp),%eax
		movl	%eax,20(%ecx)
		movl	24(%esp),%eax
		movl	%eax,24(%ecx)
1:
		xorl	%eax, %eax						# return 0
		ret

 # To be done:
ENTRY(astoff)
		ret
