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

#include "npx.h"
#include "assym.h"
#include "lapic.h"
#include "ioapic.h"
#include "ksyms.h"

#include <sys/errno.h>
#include <sys/syscall.h>

#include <machine/asm.h>

#include "machine/cputypes.h"
#include "machine/intr.h"
#include "machine/psl.h"
#include "machine/segments.h"
#include "machine/trap.h"
#include "machine/specialreg.h"

/**********************************************************************/
/* Some Handy Macros */

#define	_RELOC(x)	((x) - KERNBASE)
#define	RELOC(x)	_RELOC(_C_LABEL(x))

#define	TRAP(a)		pushl $(a) ; jmp alltraps
#define	ZTRAP(a)	pushl $0 ; TRAP(a)

/**********************************************************************/

/*
 * Compiled KERNBASE location and the kernel load address, now identical.
 */
		.globl	kernbase
		.set	kernbase, _C_LABEL(KERNBASE)
		.globl	kernload
		.set	kernload, _C_LABEL(KERNLOAD)
		
/*
 * Globals
 */
 		.data
 		ALIGN_DATA
 		.globl	_C_LABEL(esym)
 		.globl	_C_LABEL(biosbasemem),_C_LABEL(biosextmem)
 		
_C_LABEL(esym):		.long	0	# ptr to end of syms	
_C_LABEL(biosbasemem):	.long	0	# base memory reported by BIOS
_C_LABEL(biosextmem):	.long	0	# extended memory reported by BIOS

 		.globl	tmpstk
		.space	0x2000						/* space for tmpstk - temporary stack */
tmpstk:	

		.globl	bootinfo
bootinfo:	.space	BOOTINFO_SIZE				/* bootinfo that we can handle */
		
		.text
			
/**********************************************************************/
/* Initialization */

ENTRY(start)
		movw	$0x1234,0x472							# warm boot
		
/* Set up a real frame in case the double return in newboot is executed. */
		xorl	%ebp,%ebp
		pushl	%ebp
		movl	%esp, %ebp
		
/* First, reset the PSL. */
		pushl	$PSL_MBO
		popfl
		
/*
 * Don't trust what the BIOS gives for %fs and %gs.  Trust the bootstrap
 * to set %cs, %ds, %es and %ss.
 */
		mov	%ds, %ax
		mov	%ax, %fs
		mov	%ax, %gs
		
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
		call	_C_LABEL(pmap_cold)
		
		/* set up bootstrap stack */
		movl	_C_LABEL(proc0kstack),%eax			/* location of in-kernel stack */
	
		/*
		 * Only use bottom page for init386().  init386() calculates the
		 * PCB + FPU save area size and returns the true top of stack.
		 */
		leal	PAGE_SIZE(%eax),%esp

		xorl	%ebp,%ebp							/* mark end of frames */

		pushl	physfree							/* value of first for init386(first) */
		call	_C_LABEL(init386)					/* wire 386 chip for unix operation */
		
		/*
		 * Clean up the stack in a way that db_numargs() understands, so
		 * that backtraces in ddb don't underrun the stack.  Traps for
		 * inaccessible memory are more fatal than usual this early.
		 */
		addl	$4,%esp
	
		/* Switch to true top of stack. */
		movl	%eax,%esp
		
		call 	_C_LABEL(main)						/* autoconfiguration, mountroot etc */
		addl	$0,%esp								/* for db_numargs() again */
		
		.globl	__ucodesel,__udatasel
		movzwl	__ucodesel,%eax
		movzwl	__udatasel,%ecx
		
		# build outer stack frame
		pushl	%ecx								# user ss
		pushl	$USRSTACK							# user esp
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
		jnz	try486

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
		jnz	trycpuid
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
		leal	12(%esp),%eax		# get pointer to sigcontext
		movl	%eax,4(%esp)		# put it in the argument slot
									# fake return address already there
		movl	$SYS_sigreturn,%eax
		int		$0x80	 			# enter kernel with args on stack
		movl	$SYS_rexit,%eax
		int		$0x80				# exit if sigreturn fails
		
		ALIGN_TEXT
esigcode:
		.data
		.globl	szsigcode
szsigcode:
		.long	esigcode-sigcode

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
		pushl	$7		# size of instruction for restart
		jmp	syscall1

/*
 * Call gate entry for syscall
 */
IDTVEC(syscall)
		pushl	$2									/* size of instruction for restart */
syscall1:
		pushl	$T_ASTFLT	# trap # for doing ASTs
		INTRENTRY
#ifdef DIAGNOSTIC
		movl	_C_LABEL(cpl),%ebx
#endif /* DIAGNOSTIC */
		call	_C_LABEL(syscall)
2:		/* Check for ASTs on exit to user mode. */
		cli
		cmpb	$0,_C_LABEL(astpending)
		je		1f
		/* Always returning to user mode here. */
		movb	$0,_C_LABEL(astpending)
		sti
		/* Pushed T_ASTFLT into tf_trapno on entry. */
		call	_C_LABEL(trap)
		jmp		2b
#ifndef DIAGNOSTIC
1:		INTRFASTEXIT
#else /* DIAGNOSTIC */
1:		cmpl	_C_LABEL(cpl),%ebx
		jne		3f
		INTRFASTEXIT
3:		sti
		pushl	$4f
		call	_C_LABEL(printf)
		addl	$4,%esp
#ifdef DDB
		int	$3
#endif /* DDB */
		movl	%ebx,_C_LABEL(cpl)
		jmp		2b
4:		.asciz	"WARNING: SPL NOT LOWERED ON SYSCALL EXIT\n"
#endif /* DIAGNOSTIC */

/**********************************************************************/
