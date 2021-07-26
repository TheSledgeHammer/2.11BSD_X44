/*
 * Copyright (c) 1996, by Steve Passe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#define PUSH_FRAME														 \
		pushl	$0 			/* dummy error code */						;\
		pushl	$0 			/* dummy trap type */						;\
		pushal 															;\
		pushl	%ds 		/* save data and extra segments ... */		;\
		pushl	%es 													;\
		pushl	%fs

#define POP_FRAME														 \
		popl	%fs 													;\
		popl	%es 													;\
		popl	%ds 													;\
		popal 															;\
		addl	$4+4,%esp
		
#define SET_KERNEL_SREGS												 \
		movl	$KDSEL, %eax	/* reload with kernel's data segment */	;\
		movl	%eax, %ds												;\
		movl	%eax, %es												;\
		movl	$KPSEL, %eax	/* reload with per-CPU data segment */	;\
		movl	%eax, %fs

#define KENTER															 \
		testl	$PSL_VM, TF_EFLAGS(%esp)								;\
		jz		1f														;\
1:																		 \
		testb	$SEL_RPL_MASK, TF_CS(%esp)								;\
		jz		3f														;\
3:																		 \


ENTRY(lapic_eoi)
		cmpl	$0,_C_LABEL(x2apic_mode)											
		jne		1f														
		movl	_C_LABEL(local_apic_va),%eax							
		movl	$0,LAPIC_EOI(%eax)										
1:																		
		movl	$(MSR_X2APIC_BASE + MSR_X2APIC_EOI),%ecx 				
		xorl	%eax,%eax												
		xorl	%edx,%edx												
		wrmsr
		ret
		
#ifdef SMP

/*
 * Global address space TLB shootdown.
 */
IDTVEC(invltlb)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invltlb_handler, %eax
		call	*%eax	
		call	lapic_eoi
		jmp		_C_LABEL(Xdoreti)
		
/*
 * Single page TLB shootdown
 */
IDTVEC(invlpg)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invlpg_handler, %eax
		call	*%eax
		call	lapic_eoi
		jmp		_C_LABEL(Xdoreti)

/*
 * Page range TLB shootdown.
 */
IDTVEC(invlrng)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invlrng_handler, %eax
		call	*%eax
		call	lapic_eoi
		jmp		_C_LABEL(Xdoreti)

/*
 * Invalidate cache.
 */
IDTVEC(invlcache)
		PUSH_FRAME
		SET_KERNEL_SREGS
		cld
		KENTER
		movl	$invlcache_handler, %eax
		call	*%eax
		call	lapic_eoi
		jmp		_C_LABEL(Xdoreti)