/*-
 * Copyright (C) 1989, 1990 W. Jolitz
 * Copyright (c) 1992, 1993
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
 *	@(#)icu.s	8.1 (Berkeley) 6/11/93
 */

#include <net/netisr.h>

		.data
		.globl	_C_LABEL(imen),_C_LABEL(cpl),_C_LABEL(ipending)
		.globl	_C_LABEL(astpending),_C_LABEL(netisr)
		.globl	_C_LABEL(highmask), _C_LABEL(ttymask), _C_LABEL(biomask), _C_LABEL(netmask)
		.globl	_C_LABEL(isa_intr)
_C_LABEL(cpl):
		.long	0xffff			# current priority level (all off)
_C_LABEL(imen):
		.long	0xffff			# interrupt mask enable (all off)			
_C_LABEL(highmask):
		.long	0xffff
_C_LABEL(ttymask):	
		.long	0
_C_LABEL(biomask):	
		.long	0
_C_LABEL(netmask):	
		.long	0
_C_LABEL(isa_intr):
		.space	16*4

		.text

#define DONET(s, c) 				\
	.globl	c   					:\
	btrl	$ s ,_C_LABEL(netisr) 	;\
	jnb		1f  					:\
	call	c  						:\
1:

/*
 * Handle return from interrupt after device handler finishes
 *
 * register usage:
 *
 * %ebx is cpl we are going back to
 * %esi is 0 if returning to kernel mode
 *
 * Note that these registers will be preserved though C calls,
 * such as the network interrupt routines.
 */
doreti:
	cli
	popl	%ebx			# flush unit number
	popl	%ebx			# get previous priority
	# now interrupt frame is a trap frame!

	/* compensate for drivers that return with non-zero cpl */
	movl	0x34(%esp), %esi /* cs */
	andl	$3, %esi
	jz		1f

return_to_user_mode: /* entry point from trap and syscall return */

	/* return cs is for user mode: force 0 cpl */
	xorl	%ebx,%ebx
1:

	/* like splx(%ebx), except without special 0 handling */
	cli
	movl	%ebx, %eax
	movw	%ax,_C_LABEL(cpl)
	orw		_C_LABEL(imen),%ax
	outb	%al, $ IO_ICU1+1
	movb	%ah, %al
	outb	%al, $ IO_ICU2+1

	/* return immediately if previous cpl was non-zero */
	cmpw	$0, %bx
	jnz		just_return

	/* do network stuff, if requested, even if returning to kernel mode */
	cmpl	$0,_C_LABEL(netisr)
	jne		donet

	/* if (returning to user mode && astpending), go back to trap
	 * (check astpending first since it is more likely to be false)
	 */
	call	aston
	je		just_return

	testl	%esi, %esi
	jz		just_return

	/* we need to go back to trap */
	popl	%es
	popl	%ds
	popal
	addl	$8,%esp

	pushl	$0
	TRAP(T_ASTFLT)
	/* this doesn't return here ... instead it goes though
	 * calltrap in locore.s
	 */

donet:
	/* like splnet(), except we know the current pri is 0 */
	cli
	movw 	_C_LABEL(netmask), %ax
	movw 	%ax,_C_LABEL(cpl)
	orw 	_C_LABEL(imen),%ax
	outb	%al, $ IO_ICU1+1
	movb 	%ah, %al
	outb 	%al, $ IO_ICU2+1
	sti

	DONET(NETISR_RAW,_rawintr)
#ifdef INET
	DONET(NETISR_IP,_ipintr)
	DONET(NETISR_ARP,_arpintr)
#endif
#ifdef IMP
	DONET(NETISR_IMP,_impintr)
#endif
#ifdef NS
	DONET(NETISR_NS,_nsintr)
#endif
#ifdef ISO
	DONET(NETISR_ISO,_clnlintr)
#endif
#ifdef CCITT
	DONET(NETISR_CCITT,_hdintr)
#endif

	btrl	$ NETISR_SCLK,_netisr
	jnb		return_to_user_mode

	/* like splsoftclock */
	cli
	movw 	$0x8000, %ax
	movw 	%ax,_C_LABEL(cpl)
	orw 	_C_LABEL(imen),%ax
	outb 	%al, $ IO_ICU1+1
	movb 	%ah, %al
	outb 	%al, $ IO_ICU2+1
	sti

	# back to an interrupt frame for a moment
	pushl	%eax
	pushl	$0xff	# dummy intr
	call	_C_LABEL(softclock)
	leal	8(%esp), %esp
	jmp		return_to_user_mode

just_return:
	pop		%es
	pop		%ds
	popa
	leal	8(%esp),%esp
	iret

/*
 * Interrupt priority mechanism
 *
 * Two flavors	-- imlXX masks relative to ISA noemenclature (for PC compat sw)
 *		-- splXX masks with group mechanism for BSD purposes
 */

	.globl	_C_LABEL(splhigh), _C_LABEL(splclock)
	.globl	_splclock
_C_LABEL(splhigh):
_C_LABEL(splclock):
	cli								# disable interrupts
	movw	$0xffff,%ax				# set new priority level
	movw	%ax,%dx
	# orw	_imen,%ax			# mask off those not enabled yet
	movw	%ax,%cx
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	movzwl	_C_LABEL(cpl),%eax		# return old priority
	movw	%dx,_C_LABEL(cpl)		# set new priority level
	sti								# enable interrupts
	ret

	.globl	_C_LABEL(spltty)		# block clists
_C_LABEL(spltty):
	cli								# disable interrupts
	movw	_C_LABEL(cpl),%ax
	orw		_C_LABEL(ttymask),%ax
	movw	%ax,%dx
	orw		_C_LABEL(imen),%ax		# mask off those not enabled yet
	movw	%ax,%cx
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	movzwl	_C_LABEL(cpl),%eax		# return old priority
	movw	%dx,_C_LABEL(cpl)		# set new priority level
	sti								# enable interrupts
	ret

	.globl	_C_LABEL(splimp), _C_LABEL(splnet)
_C_LABEL(splimp):
_C_LABEL(splnet):
	cli								# disable interrupts
	movw	_C_LABEL(cpl),%ax
	orw		_C_LABEL(netmask),%ax
	movw	%ax,%dx
	orw		_C_LABEL(imen),%ax		# mask off those not enabled yet
	movw	%ax,%cx
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	movzwl	_C_LABEL(cpl),%eax		# return old priority
	movw	%dx,_C_LABEL(cpl)		# set new priority level
	sti								# enable interrupts
	ret

	.globl	_C_LABEL(splbio)	
_C_LABEL(splbio):
	cli								# disable interrupts
	movw	_C_LABEL(cpl),%ax
	orw		_C_LABEL(biomask),%ax
	movw	%ax,%dx
	orw		_C_LABEL(imen),%ax		# mask off those not enabled yet
	movw	%ax,%cx
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	movzwl	_C_LABEL(cpl),%eax		# return old priority
	movw	%dx,_C_LABEL(cpl)		# set new priority level
	sti								# enable interrupts
	ret

	.globl	_C_LABEL(splsoftclock)
_C_LABEL(splsoftclock):
	cli								# disable interrupts
	movw	_C_LABEL(cpl),%ax
	orw		$0x8000,%ax				# set new priority level
	movw	%ax,%dx
	orw		_C_LABEL(imen),%ax		# mask off those not enabled yet
	movw	%ax,%cx
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	movzwl	_C_LABEL(cpl),%eax		# return old priority
	movw	%dx,_C_LABEL(cpl)		# set new priority level
	sti								# enable interrupts
	ret

	.globl _C_LABEL(splnone), _C_LABEL(spl0)
_C_LABEL(splnone):
_C_LABEL(spl0):
	cli								# disable interrupts
	pushl	_C_LABEL(cpl)			# save old priority
	movw	_C_LABEL(cpl),%ax
	orw		_C_LABEL(netmask),%ax	# mask off those network devices
	movw	%ax,_C_LABEL(cpl)		# set new priority level
	orw		_C_LABEL(imen),%ax		# mask off those not enabled yet
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	sti								# enable interrupts

	DONET(NETISR_RAW,_rawintr)
#ifdef INET
	DONET(NETISR_IP,_ipintr)
	DONET(NETISR_ARP,_arpintr)
#endif
#ifdef IMP
	DONET(NETISR_IMP,_impintr)
#endif
#ifdef NS
	DONET(NETISR_NS,_nsintr)
#endif
#ifdef ISO
	DONET(NETISR_ISO,_clnlintr)
#endif
#ifdef CCITT
	DONET(NETISR_CCITT,_hdintr)
#endif

	cli								# disable interrupts
	popl	_C_LABEL(cpl)			# save old priority
	nop

	movw	$0,%ax					# set new priority level
	movw	%ax,%dx
	orw		_C_LABEL(imen),%ax		# mask off those not enabled yet
	movw	%ax,%cx
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	movzwl	_C_LABEL(cpl),%eax		# return old priority
	movw	%dx,_C_LABEL(cpl)		# set new priority level
	sti								# enable interrupts
	ret

	.globl _C_LABEL(splx)
_C_LABEL(splx):
	cli								# disable interrupts
	movw	4(%esp),%ax				# new priority level
	movw	%ax,%dx
	cmpw	$0,%dx
	je		_C_LABEL(spl0)			# going to "zero level" is special

	orw		_C_LABEL(imen),%ax		# mask off those not enabled yet
	movw	%ax,%cx
	outb	%al,$ IO_ICU1+1			/* update icu's */
	movb	%ah,%al
	outb	%al,$ IO_ICU2+1
	movzwl	_C_LABEL(cpl),%eax		# return old priority
	movw	%dx,_C_LABEL(cpl)		# set new priority level
	sti								# enable interrupts
	ret