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

/*
 * locore.s: 4BSD machine support for the Intel 386
 *		Preliminary version
 *		Written by William F. Jolitz, 386BSD Project
 */

#include <sys/errno.h>
#include <sys/reboot.h>

#include <machine/asm.h>
#include <machine/psl.h>
#include <machine/pmap.h>
#include <machine/pte.h>
#include <machine/trap.h>
#include <machine/specialreg.h>
#include <machine/cpu.h>
#include <machine/cputypes.h>

#include "assym.h"

/**********************************************************************/
/* Some Handy Macros */

#define	_RELOC(x)		((x) - KERNBASE)
#define	RELOC(x)		_RELOC(_C_LABEL(x))

#define	TRAP(a)			pushl $(a) ; jmp alltraps
#define	ZTRAP(a)		pushl $0 ; TRAP(a)

#define	INTRENTRY 				  \
		pushl	%eax			; \
		pushl	%ecx			; \
		pushl	%edx			; \
		pushl	%ebx			; \
		pushl	%ebp			; \
		pushl	%esi			; \
		pushl	%edi			; \
		pushl	%ds				; \
		pushl	%es				; \
		movl	$GSEL(GDATA_SEL, SEL_KPL),%eax	; \
		movl	%ax,%ds			; \
		movl	%ax,%es

#define	INTRFASTEXIT 			  \
		popl	%es				; \
		popl	%ds				; \
		popl	%edi			; \
		popl	%esi			; \
		popl	%ebp			; \
		popl	%ebx			; \
		popl	%edx			; \
		popl	%ecx			; \
		popl	%eax			; \
		addl	$8,%esp			; \
		iret

/**********************************************************************/

/*
 * Compiled KERNBASE location and the kernel load address, now identical.
 */
		.globl	kernbase
		.set	kernbase,KERNBASE
		.globl	kernload
		.set	kernload,KERNLOAD
		
/*
 * Globals
 */
 		.data
 		ALIGN_DATA
 		.globl	tmpstk
				.space	0x2000						/* space for tmpstk - temporary stack */
tmpstk:	

		.globl	bootinfo
bootinfo:		.space	BOOTINFO_SIZE				/* bootinfo that we can handle */
		
		.text
		
/**********************************************************************/
/* Initialization */

ENTRY(start)
		movw	$0x1234,0x472							# warm boot
		jmp		1f
		.space	0x500									# skip over warm boot shit

1:		movl	%eax,RELOC(_esym)
		movl	20(%esp),%eax
		movl	%eax,RELOC(_biosextmem)
		movl	24(%esp),%eax
		movl	%eax,RELOC(_biosbasemem)

		/* First, reset the PSL. */
		pushl	$PSL_MBO
		popfl
		
/*
 * Don't trust what the BIOS gives for %fs and %gs.  Trust the bootstrap
 * to set %cs, %ds, %es and %ss.
 */
		mov		%ds, %ax
		mov		%ax, %fs
		mov		%ax, %gs
		
		/*
 * Clear the bss.  Not all boot programs do it, and it is our job anyway.
 *
 * XXX we don't check that there is memory for our bss and page tables
 * before using it.
 *
 * Note: we must be careful to not overwrite an active gdt or idt.  They
 * inactive from now until we switch to new ones, since we don't load any
 * more segment registers or permit interrupts until after the switch.
 */
		movl	$__bss_end,%ecx
		movl	$__bss_start,%edi
		subl	%edi,%ecx
		xorl	%eax,%eax
		cld
		rep
		stosb

		call	recover_bootinfo
		
/* Get onto a stack that we can trust. */
/*
 * XXX this step is delayed in case recover_bootinfo needs to return via
 * the old stack, but it need not be, since recover_bootinfo actually
 * returns via the old frame.
 */
 		movl	$tmpstk,%esp

		call	identify_cpu
		call	pmap_cold
		
		/* set up bootstrap stack */
		movl	_C_LABEL(proc0kstack),%eax			/* location of in-kernel stack */
	
		/*
		 * Only use bottom page for init386().  init386() calculates the
		 * PCB + FPU save area size and returns the true top of stack.
		 */
		leal	PAGE_SIZE(%eax),%esp

		xorl	%ebp,%ebp							/* mark end of frames */

		pushl	physfree							/* value of first for init386(first) */
		call	init386								/* wire 386 chip for unix operation */
		
		/*
		 * Clean up the stack in a way that db_numargs() understands, so
		 * that backtraces in ddb don't underrun the stack.  Traps for
		 * inaccessible memory are more fatal than usual this early.
		 */
		addl	$4,%esp
	
		/* Switch to true top of stack. */
		movl	%eax,%esp
		

		call 	main								/* autoconfiguration, mountroot etc */
		
		.globl	__ucodesel,__udatasel
		movzwl	__ucodesel,%eax
		movzwl	__udatasel,%ecx
		
		# build outer stack frame
		pushl	%ecx								# user ss
		pushl	$ USRSTACK							# user esp
		pushl	%eax								# user cs
		pushl	$0									# user ip
		movw	%cx,%ds
		movw	%cx,%es
		movw	%ax,%fs								# double map cs to fs
		movw	%cx,%gs								# and ds to gs
		lret										# goto user!

		pushl	$lretmsg1							/* "should never get here!" */
		call	_C_LABEL(panic)
lretmsg1:
		.asciz	"lret: toinit\n"

		
/**********************************************************************/
/* Recover the Bootinfo */

recover_bootinfo:
		/*
		 * This code is called in different ways depending on what loaded
		 * and started the kernel.  This is used to detect how we get the
		 * arguments from the other code and what we do with them.
		 *
		 * Old disk boot blocks:
		 *	(*btext)(howto, bootdev, cyloffset, esym);
		 *	[return address == 0, and can NOT be returned to]
		 *	[cyloffset was not supported by the FreeBSD boot code
		 *	 and always passed in as 0]
		 *	[esym is also known as total in the boot code, and
		 *	 was never properly supported by the FreeBSD boot code]
		 *
		 * Old diskless netboot code:
		 *	(*btext)(0,0,0,0,&nfsdiskless,0,0,0);
		 *	[return address != 0, and can NOT be returned to]
		 *	If we are being booted by this code it will NOT work,
		 *	so we are just going to halt if we find this case.
		 *
		 * New uniform boot code:
		 *	(*btext)(howto, bootdev, 0, 0, 0, &bootinfo)
		 *	[return address != 0, and can be returned to]
		 *
		 * There may seem to be a lot of wasted arguments in here, but
		 * that is so the newer boot code can still load very old kernels
		 * and old boot code can load new kernels.
		 */
	
		/*
		 * The old style disk boot blocks fake a frame on the stack and
		 * did an lret to get here.  The frame on the stack has a return
		 * address of 0.
		 */
		cmpl	$0,4(%ebp)
		je		olddiskboot
	
		/*
		 * We have some form of return address, so this is either the
		 * old diskless netboot code, or the new uniform code.  That can
		 * be detected by looking at the 5th argument, if it is 0
		 * we are being booted by the new uniform boot code.
		 */
		cmpl	$0,24(%ebp)
		je		newboot
	
		/*
		 * Seems we have been loaded by the old diskless boot code, we
		 * don't stand a chance of running as the diskless structure
		 * changed considerably between the two, so just halt.
		 */
		 hlt
	
		/*
		 * We have been loaded by the new uniform boot code.
		 * Let's check the bootinfo version, and if we do not understand
		 * it we return to the loader with a status of 1 to indicate this error
		 */
newboot:
		movl	28(%ebp),%ebx			/* &bootinfo.version */
		movl	BI_VERSION(%ebx),%eax
		cmpl	$1,%eax					/* We only understand version 1 */
		je		1f
		movl	$1,%eax					/* Return status */
		leave
		/*
		 * XXX this returns to our caller's caller (as is required) since
		 * we didn't set up a frame and our caller did.
		 */
		ret

1:
		/*
		 * If we have a kernelname copy it in
		 */
		movl	BI_KERNELNAME(%ebx),%esi
		cmpl	$0,%esi
		je		2f						/* No kernelname */
		movl	$MAXPATHLEN,%ecx		/* Brute force!!! */
		movl	$kernelname,%edi
		cmpb	$'/',(%esi)				/* Make sure it starts with a slash */
		je		1f
		movb	$'/',(%edi)
		incl	%edi
		decl	%ecx
1:
		cld
		rep
		movsb

2:
		/*
		 * Determine the size of the boot loader's copy of the bootinfo
		 * struct.  This is impossible to do properly because old versions
		 * of the struct don't contain a size field and there are 2 old
		 * versions with the same version number.
		 */
		movl	$BI_ENDCOMMON,%ecx		/* prepare for sizeless version */
		testl	$RB_BOOTINFO,8(%ebp)	/* bi_size (and bootinfo) valid? */
		je		got_bi_size				/* no, sizeless version */
		movl	BI_SIZE(%ebx),%ecx
got_bi_size:
	
		/*
		 * Copy the common part of the bootinfo struct
		 */
		movl	%ebx,%esi
		movl	$bootinfo,%edi
		cmpl	$BOOTINFO_SIZE,%ecx
		jbe		got_common_bi_size
		movl	$BOOTINFO_SIZE,%ecx
got_common_bi_size:
		cld
		rep
		movsb

#ifdef NFS_ROOT
#ifndef BOOTP_NFSV3
		/*
		 * If we have a nfs_diskless structure copy it in
		 */
		movl	BI_NFS_DISKLESS(%ebx),%esi
		cmpl	$0,%esi
		je		olddiskboot
		movl	$nfs_diskless,%edi
		movl	$NFSDISKLESS_SIZE,%ecx
		cld
		rep
		movsb
		movl	$nfs_diskless_valid,%edi
		movl	$1,(%edi)
#endif
#endif

		/*
		 * The old style disk boot.
		 *	(*btext)(howto, bootdev, cyloffset, esym);
		 * Note that the newer boot code just falls into here to pick
		 * up howto and bootdev, cyloffset and esym are no longer used
		 */
olddiskboot:
		movl	8(%ebp),%eax
		movl	%eax,boothowto
		movl	12(%ebp),%eax
		movl	%eax,bootdev
	
		ret

/**********************************************************************/
/* Identify CPU's */

ENTRY(identify_cpu)

		pushl	%ebx

		/* Try to toggle alignment check flag; does not exist on 386. */
		pushfl
		popl	%eax
		movl	%eax,%ecx
		orl		$PSL_AC,%eax
		pushl	%eax
		popfl
		pushfl
		popl	%eax
		xorl	%ecx,%eax
		andl	$PSL_AC,%eax
		pushl	%ecx
		popfl

		testl	%eax,%eax
		jnz		try486

		/* NexGen CPU does not have aligment check flag. */
		pushfl
		movl	$0x5555, %eax
		xorl	%edx, %edx
		movl	$2, %ecx
		clc
		divl	%ecx
		jz		trynexgen
		popfl
		movl	$CPU_386,_C_LABEL(cpu)
		jmp		3f

trynexgen:
		popfl
		movl	$CPU_NX586,_C_LABEL(cpu)
		movl	$0x4778654e,_C_LABEL(cpu_vendor)	# store vendor string
		movl	$0x72446e65,_C_LABEL(cpu_vendor)+4
		movl	$0x6e657669,_C_LABEL(cpu_vendor)+8
		movl	$0,_C_LABEL(cpu_vendor)+12
		jmp		3f

try486:	/* Try to toggle identification flag; does not exist on early 486s. */
		pushfl
		popl	%eax
		movl	%eax,%ecx
		xorl	$PSL_ID,%eax
		pushl	%eax
		popfl
		pushfl
		popl	%eax
		xorl	%ecx,%eax
		andl	$PSL_ID,%eax
		pushl	%ecx
		popfl

		testl	%eax,%eax
		jnz		trycpuid
		movl	$CPU_486,_C_LABEL(cpu)

		/*
		 * Check Cyrix CPU
		 * Cyrix CPUs do not change the undefined flags following
		 * execution of the divide instruction which divides 5 by 2.
		 *
		 * Note: CPUID is enabled on M2, so it passes another way.
		 */
		pushfl
		movl	$0x5555, %eax
		xorl	%edx, %edx
		movl	$2, %ecx
		clc
		divl	%ecx
		jnc		trycyrix
		popfl
		jmp		3f		/* You may use Intel CPU. */

trycyrix:
		popfl
		/*
		 * IBM Bluelighting CPU also doesn't change the undefined flags.
		 * Because IBM doesn't disclose the information for Bluelighting
		 * CPU, we couldn't distinguish it from Cyrix's (including IBM
		 * brand of Cyrix CPUs).
		 */
		movl	$0x69727943,_C_LABEL(cpu_vendor)	# store vendor string
		movl	$0x736e4978,_C_LABEL(cpu_vendor)+4
		movl	$0x64616574,_C_LABEL(cpu_vendor)+8
		jmp		3f

trycpuid:	/* Use the `cpuid' instruction. */
		xorl	%eax,%eax
		cpuid										# cpuid 0
		movl	%eax,_C_LABEL(cpu_high)				# highest capability
		movl	%ebx,_C_LABEL(cpu_vendor)			# store vendor string
		movl	%edx,_C_LABEL(cpu_vendor)+4
		movl	%ecx,_C_LABEL(cpu_vendor)+8
		movb	$0,_C_LABEL(cpu_vendor)+12

		movl	$1,%eax
		cpuid										# cpuid 1
		movl	%eax,_C_LABEL(cpu_id)				# store cpu_id
		movl	%ebx,_C_LABEL(cpu_procinfo)			# store cpu_procinfo
		movl	%edx,_C_LABEL(cpu_feature)			# store cpu_feature
		movl	%ecx,_C_LABEL(cpu_feature2)			# store cpu_feature2
		rorl	$8,%eax								# extract family type
		andl	$15,%eax
		cmpl	$5,%eax
		jae		1f

		/* less than Pentium; must be 486 */
		movl	$CPU_486,_C_LABEL(cpu)
		jmp		3f
1:
		/* a Pentium? */
		cmpl	$5,%eax
		jne		2f
		movl	$CPU_586,_C_LABEL(cpu)
		jmp		3f
2:
		/* Greater than Pentium...call it a Pentium Pro */
		movl	$CPU_686,_C_LABEL(cpu)
3:
		popl	%ebx
		ret
		
/**********************************************************************/
/*
 * Signal trampoline, copied to top of user stack
 */

ENTRY(sigcode)
		movl	12(%esp),%eax						# unsure if call will dec stack 1st
		call	%eax
		xorl	%eax,%eax							# smaller movl $103,%eax
		movb	$103,%al							# sigreturn()
		LCALL(0x7,0)								# enter kernel with args on stack
		hlt											# never gets here

esigcode:
		.data
		.globl	szsigcode

szsigcode:
		.long	szsigcode-sigcode

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
 * Setrq(p)
 *
 * Call should be made at spl6(), and p->p_stat should be SRUN
 */
ENTRY(setrq)
		movl	4(%esp),%eax
		cmpl	$0,P_BACK(%eax)						# should not be on q already
		je		set1
		pushl	$set2
		call	_C_LABEL(panic)
set1:
		movzbl	P_PRIORITY(%eax),%edx
		shrl	$2,%edx
		btsl	%edx,_C_LABEL(whichqs)				# set q full bit
		shll	$3,%edx
		addl	$_qs,%edx							# locate q hdr
		movl	%edx,P_FORW(%eax)					# link process on tail of q
		movl	P_BACK(%edx),%ecx
		movl	%ecx,P_BACK(%eax)
		movl	%eax,P_BACK(%edx)
		movl	%eax,P_FORW(%ecx)
		ret

set2:	.asciz	"setrq"

/*
 * Remrq(p)
 *
 * Call should be made at spl6().
 */
ENTRY(remrq)
		movl	4(%esp),%eax
		movzbl	P_PRIORITY(%eax),%edx
		shrl	$2,%edx
		btrl	%edx,_C_LABEL(whichqs)				# clear full bit, panic if clear already
		jb		rem1
		pushl	$rem3
		call	_C_LABEL(panic)
rem1:
		pushl	%edx
		movl	P_FORW(%eax),%ecx					# unlink process
		movl	P_BACK(%eax),%edx
		movl	%edx,P_BACK(%ecx)
		movl	P_BACK(%eax),%ecx
		movl	P_FORW(%eax),%edx
		movl	%edx,P_FORW(%ecx)
		popl	%edx
		movl	$_C_LABEL(qs),%ecx
		shll	$3,%edx
		addl	%edx,%ecx
		cmpl	P_FORW(%ecx),%ecx					# q still has something?
		je		rem2
		shrl	$3,%edx								# yes, set bit as still full
		btsl	%edx,_C_LABEL(whichqs)
rem2:
		movl	$0,P_BACK(%eax)						# zap reverse link to indicate off list
		ret

rem3:	.asciz	"remrq"
sw0:	.asciz	"swtch"

/*
 * When no processes are on the runq, Swtch branches to idle
 * to wait for something to come ready.
 */
ENTRY(idle)
		call	_C_LABEL(spl0)
		cmpl	$0,_C_LABEL(whichqs)
		jne		sw1
		hlt		# wait for interrupt
		jmp		idle

		.align 	4 									/* ..so that profiling doesn't lump Idle with Xswitch().. */
badsw:
		pushl	$sw0
		call	_C_LABEL(panic)
		/*NOTREACHED*/

/*
 * Swtch()
 */
ENTRY(swtch)

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

		movl	CMAP2,%eax							# save temporary map PTE
		movl	%eax,PCB_CMAP2(%ecx)				# in our context
		movl	$0,_C_LABEL(curproc)				# out of process

		# movw	_cpl, %ax
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

/*
 * struct proc *switch_to_inactive(p) ; struct proc *p;
 *
 * At exit of a process, move off the address space of the
 * process and onto a "safe" one. Then, on a temporary stack
 * return and run code that disposes of the old state.
 * Since this code requires a parameter from the "old" stack,
 * pass it back as a return value.
 */

ENTRY(switch_to_inactive)
		popl	%edx								# old pc
		popl	%eax								# arg, our return value
		movl	_IdlePTD,%ecx
		movl	%ecx,%cr3							# good bye address space
 #write buffer?
		movl	$tmpstk-4,%esp						# temporary stack, compensated for call
		jmp		%edx								# return, execute remainder of cleanup

/*
 * savectx(pcb, altreturn)
 * Update pcb, saving current processor state and arranging
 * for alternate return ala longjmp in Xswitch if altreturn is true.
 */
ENTRY(savectx)
		movl	4(%esp), %ecx
		movw	_C_LABEL(cpl), %ax
		movw	%ax,  PCB_IML(%ecx)
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

		movl	_CMAP2, %edx					# save temporary map PTE
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

/*
 * addupc(int pc, struct uprof *up, int ticks):
 * update profiling information for the user process.
 */

ENTRY(addupc)
		pushl 	%ebp
		movl 	%esp,%ebp
		movl 	12(%ebp),%edx					/* up */
		movl 	8(%ebp),%eax					/* pc */

		subl 	PR_OFF(%edx),%eax				/* pc -= up->pr_off */
		jl 		L1								/* if (pc < 0) return */

		shrl 	$1,%eax							/* praddr = pc >> 1 */
		imull	PR_SCALE(%edx),%eax				/* praddr *= up->pr_scale */
		shrl 	$15,%eax						/* praddr = praddr << 15 */
		andl 	$-2,%eax						/* praddr &= ~1 */

		cmpl 	PR_SIZE(%edx),%eax				/* if (praddr > up->pr_size) return */
		ja 		L1

/*		addl 	%eax,%eax						/* praddr -> word offset */
		addl 	PR_BASE(%edx),%eax				/* praddr += up-> pr_base */
		movl 	16(%ebp),%ecx					/* ticks */

		movl 	_C_LABEL(curpcb),%edx
		movl 	$_C_LABEL(proffault),PCB_ONFAULT(%edx)
		addl 	%ecx,(%eax)						/* storage location += ticks */
		movl 	$0,PCB_ONFAULT(%edx)
L1:
		leave
		ret

proffault:
/* if we get a fault, then kill profiling all together */
		movl 	$0,PCB_ONFAULT(%edx)			/* squish the fault handler */
 		movl 	12(%ebp),%ecx
		movl 	$0,PR_SCALE(%ecx)				/* up->pr_scale = 0 */
		leave
		ret

LF:		.asciz "swtch %x"

.text
 # To be done:
		.globl _astoff
_astoff:
		ret


/**********************************************************************/
/*
 * Trap and fault vector routines
 */
		.text

IDTVEC(div)
		ZTRAP(T_DIVIDE)
IDTVEC(dbg)
		ZTRAP(T_TRCTRAP)
IDTVEC(nmi)
		ZTRAP(T_NMI)
IDTVEC(bpt)
		ZTRAP(T_BPTFLT)
IDTVEC(ofl)
		ZTRAP(T_OFLOW)
IDTVEC(bnd)
		ZTRAP(T_BOUND)
IDTVEC(ill)
		ZTRAP(T_PRIVINFLT)
IDTVEC(dna)
		ZTRAP(T_DNA)
IDTVEC(dble)
		TRAP(T_DOUBLEFLT)
IDTVEC(fpusegm)
		ZTRAP(T_FPOPFLT)
IDTVEC(tss)
		TRAP(T_TSSFLT)
IDTVEC(missing)
		TRAP(T_SEGNPFLT)
IDTVEC(stk)
		TRAP(T_STKFLT)
IDTVEC(prot)
		TRAP(T_PROTFLT)
IDTVEC(page)
		TRAP(T_PAGEFLT)
IDTVEC(fpu)
#ifdef NPX
/*
 * Handle like an interrupt so that we can call npxintr to clear the
 * error.  It would be better to handle npx interrupts as traps but
 * this is difficult for nested interrupts.
 */
		pushl	$0									/* dummy error code */
		pushl	$T_ASTFLT
		pushal
		nop											/* silly, the bug is for popal and it only
				 		 				 			 * bites when the next instruction has a
				 		 				 			 * complicated address mode
				 		 				 			 */
		pushl	%ds
		pushl	%es									/* now the stack frame is a trap frame */
		movl	$GSEL(GDATA_SEL, SEL_KPL),%eax
		movl	%ax,%ds
		movl	%ax,%es
		pushl	_C_LABEL(cpl)
		pushl	$0									/* dummy unit to finish building intr frame */
		incl	_C_LABEL(cnt)+V_TRAP
		call	_C_LABEL(npxintr)
		jmp		_C_LABEL(Xdoreti)
#else
		ZTRAP(T_ARITHTRAP)
#endif
IDTVEC(align)
		ZTRAP(T_ALIGNFLT)
/* 18 - 31 reserved for future exp */
IDTVEC(rsvd)	/* rvsd 18 */
		ZTRAP(T_RESERVED)

ENTRY(alltraps)
		pushal
		nop
		push 	%ds
		push 	%es
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
calltrap:
		incl	_C_LABEL(cnt)+V_TRAP
		call	_C_LABEL(trap)
/*
 * Return through doreti to handle ASTs.  Have to change trap frame
 * to interrupt frame.
 */
		movl	$T_ASTFLT,4+4+32(%esp)				/* new trap type (err code not used) */
		pushl	_C_LABEL(cpl)
		pushl	$0									/* dummy unit */
		jmp		_C_LABEL(Xdoreti)

/*
 * Old call gate entry for syscall
 */
IDTVEC(osyscall)
		/* Set eflags in trap frame. */
		pushfl
		popl	8(%esp)
		/* Turn off trace flag and nested task. */
		pushfl
		andb	$~((PSL_T|PSL_NT)>>8),1(%esp)
		popfl
		pushl	$7									/* size of instruction for restart	*/
		jmp		syscall1

/*
 * Call gate entry for syscall
 */

IDTVEC(syscall)
		pushl	$2									/* size of instruction for restart */
syscall1:
		pushfl										/* only for stupid carry bit and more stupid wait3 cc kludge */
		pushal										/* only need eax,ecx,edx - trap resaves others */
		nop
		movl	$GSEL(GDATA_SEL, SEL_KPL),%eax		/* switch to kernel segments */
		movl	%ax,%ds
		movl	%ax,%es
		incl	_C_LABEL(cnt)+V_SYSCALL  			/* kml 3/25/93 */
		call	_C_LABEL(syscall)
/*
 * Return through doreti to handle ASTs.  Have to change syscall frame
 * to interrupt frame.
 *
 * XXX - we should have set up the frame earlier to avoid the
 * following popal/pushal (not much can be done to avoid shuffling
 * the flags).  Consistent frames would simplify things all over.
 */
		movl	32+0(%esp),%eax						/* old flags, shuffle to above cs:eip */
		movl	32+4(%esp),%ebx						/* `int' frame should have been ef, eip, cs */
		movl	32+8(%esp),%ecx
		movl	%ebx,32+0(%esp)
		movl	%ecx,32+4(%esp)
		movl	%eax,32+8(%esp)
		popal
		nop
		pushl	$0									/* dummy error code */
		pushl	$T_ASTFLT
		pushal
		nop
		movl	__udatasel,%eax						/* switch back to user segments */
		push	%eax								/* XXX - better to preserve originals? */
		push	%eax
		pushl	_C_LABEL(cpl)
		pushl	$0
		jmp		_C_LABEL(Xdoreti)

/**********************************************************************/
