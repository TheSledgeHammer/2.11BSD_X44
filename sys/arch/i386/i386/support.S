/*-
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
 *
 */

#include <machine/asm.h>
#include <machine/cputypes.h>
#include <machine/specialreg.h>

#include "assym.h"

#define	IDXSHIFT 	10

#define	NOP				\
	inb 	$0x84, %al;	\
	inb 	$0x84, %al

/*
 * Support routines for GCC
 */
ENTRY(udivsi3)
		movl 	4(%esp),%eax
		xorl 	%edx,%edx
		divl 	8(%esp)
		ret

ENTRY(divsi3)
		movl 	4(%esp),%eax
		#xorl 	%edx,%edx		/* not needed - cltd sign extends into %edx */
		cltd
		idivl 	8(%esp)
		ret

/*
* I/O bus instructions via C
*/

ENTRY(inb)
		movl	4(%esp),%edx
		subl	%eax,%eax		# clr eax
		NOP
		inb		%dx,%al
		ret

ENTRY(inw)
		movl	4(%esp),%edx
		subl	%eax,%eax		# clr eax
		NOP
		inw		%dx,%ax
		ret

ENTRY(rtcin)
		movl	4(%esp),%eax
		outb	%al,$0x70
		subl	%eax,%eax		# clr eax
		inb		$0x71,%al		# Compaq SystemPro
		ret

ENTRY(outb)
		movl	4(%esp),%edx
		NOP
		movl	8(%esp),%eax
		outb	%al,%dx
		NOP
		ret

ENTRY(outw)
		movl	4(%esp),%edx
		NOP
		movl	8(%esp),%eax
		outw	%ax,%dx
		NOP
		ret

/*
 * void bzero(void *base, u_int cnt)
 */

ENTRY(bzero)
		pushl	%edi
		movl	8(%esp),%edi
		movl	12(%esp),%ecx
		xorl	%eax,%eax
		shrl	$2,%ecx
		cld
		rep
		stosl
		movl	12(%esp),%ecx
		andl	$3,%ecx
		rep
		stosb
		popl	%edi
		ret

/*
 * fillw (pat,base,cnt)
 */

ENTRY(fillw)
		pushl	%edi
		movl	8(%esp),%eax
		movl	12(%esp),%edi
		movw	%ax, %cx
		rorl	$16, %eax
		movw	%cx, %ax
		cld
		movl	16(%esp),%ecx
		shrl	%ecx
		rep
		stosl
		movl	16(%esp),%ecx
		andl	$1, %ecx
		rep
		stosw
		popl	%edi
		ret

ENTRY(bcopyb)
		pushl	%esi
		pushl	%edi
		movl	12(%esp),%esi
		movl	16(%esp),%edi
		movl	20(%esp),%ecx
		cld
		rep
		movsb
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		ret

/*
 * (ov)bcopy (src,dst,cnt)
 *  ws@tools.de     (Wolfgang Solfrank, TooLs GmbH) +49-228-985800
 */

ENTRY(ovbcopy)
ALTENTRY(bcopy)
		pushl	%esi
		pushl	%edi
		movl	12(%esp),%esi
		movl	16(%esp),%edi
		movl	20(%esp),%ecx
		cmpl	%esi,%edi		/* potentially overlapping? */
		jnb		1f
		cld						/* nope, copy forwards. */
		shrl	$2,%ecx			/* copy by words */
		rep
		movsl
		movl	20(%esp),%ecx
		andl	$3,%ecx			/* any bytes left? */
		rep
		movsb
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		ret
		ALIGN_TEXT
1:
		addl	%ecx,%edi		/* copy backwards. */
		addl	%ecx,%esi
		std
		andl	$3,%ecx			/* any fractional bytes? */
		decl	%edi
		decl	%esi
		rep
		movsb
		movl	20(%esp),%ecx	/* copy remainder by words */
		shrl	$2,%ecx
		subl	$3,%esi
		subl	$3,%edi
		rep
		movsl
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		cld
		ret

#ifdef notdef
ENTRY(copyout_fast)
		movl	_C_LABEL(curpcb), %eax
		movl	$cpyflt, PCB_ONFAULT(%eax) # in case we page/protection violate
		pushl	%esi
		pushl	%edi
		pushl	%ebx
		movl	16(%esp), %esi
		movl	20(%esp), %edi
		movl	24(%esp), %ebx

 /* first, check to see if "write fault" */
1:		movl	%edi, %eax
#ifdef notyet
		shrl	$IDXSHIFT, %eax		/* fetch pte associated with address */
		andb	$0xfc, %al
		movl	_C_LABEL(PTmap)(%eax), %eax

		andb	$7, %al				/* if we are the one case that won't trap... */
		cmpb	$5, %al
		jne		2f
/* ... then simulate the trap! */
		pushl	%edi
		call	_C_LABEL(trapwrite)	/* trapwrite(addr) */
		popl	%edx

		cmpl	$0, %eax			/* if not ok, return */
		jne		cpyflt
/* otherwise, continue with reference */
2:
		movl	%edi, %eax			/* calculate remainder this pass */
		andl	$0xfffff000, %eax
		movl	$NBPG, %ecx
		subl	%eax, %ecx
		cmpl	%ecx, %ebx
		jle		3f
		movl	%ebx, %ecx
3:		subl	%ecx, %ebx
		movl	%ecx, %edx
#else
		movl	%ebx, %ecx
		movl	%ebx, %edx
#endif

		shrl	$2,%ecx				/* movem */
		cld
		rep
		movsl
		movl	%edx, %ecx			/* don't depend on ecx here! */
		andl	$3, %ecx
		rep
		movsb

#ifdef notyet
		cmpl	$0, %ebx
		jl		1b
#endif

		popl	%ebx
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		movl	_C_LABEL(curpcb),%edx
		movl	%eax,PCB_ONFAULT(%edx)
		ret

ENTRY(copyin)
		movl	_C_LABEL(curpcb),%eax
		movl	$cpyflt,PCB_ONFAULT(%eax) # in case we page/protection violate
		pushl	%esi
		pushl	%edi
		pushl	%ebx
		movl	12(%esp),%esi
		movl	16(%esp),%edi
		movl	20(%esp),%ecx
		shrl	$2,%ecx
		cld
		rep
		movsl
		movl	20(%esp),%ecx
		andl	$3,%ecx
		rep
		movsb
		popl	%ebx
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		movl	_C_LABEL(curpcb),%edx
		movl	%eax,PCB_ONFAULT(%edx)
		ret

ENTRY(cpyflt)
		popl	%ebx
		popl	%edi
		popl	%esi
		movl	_C_LABEL(curpcb),%edx
		movl	$0,PCB_ONFAULT(%edx)
		movl	$ EFAULT,%eax
		ret
#else

ENTRY(copyout_fast)
		movl	_C_LABEL(curpcb),%eax
		movl	$cpyflt,PCB_ONFAULT(%eax) # in case we page/protection violate
		pushl	%esi
		pushl	%edi
		movl	12(%esp),%esi
		movl	16(%esp),%edi
		movl	20(%esp),%ecx
		shrl	$2,%ecx
		cld
		rep
		movsl
		movl	20(%esp),%ecx
		andl	$3,%ecx
		rep
		movsb
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		movl	_C_LABEL(curpcb),%edx
		movl	%eax,PCB_ONFAULT(%edx)
		ret

ENTRY(copyin)
		movl	_C_LABEL(curpcb),%eax
		movl	$cpyflt,PCB_ONFAULT(%eax) # in case we page/protection violate
		pushl	%esi
		pushl	%edi
		movl	12(%esp),%esi
		movl	16(%esp),%edi
		movl	20(%esp),%ecx
		shrl	$2,%ecx
		cld
		rep
		movsl
		movl	20(%esp),%ecx
		andl	$3,%ecx
		rep
		movsb
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		movl	_C_LABEL(curpcb),%edx
		movl	%eax,PCB_ONFAULT(%edx)
		ret

ENTRY(cpyflt)
		popl	%edi
		popl	%esi
		movl	_C_LABEL(curpcb),%edx
		movl	$0,PCB_ONFAULT(%edx)
		movl	$ EFAULT,%eax
		ret

#endif

# insb(port,addr,cnt)

ENTRY(insb)
		pushl	%edi
		movw	8(%esp),%dx
		movl	12(%esp),%edi
		movl	16(%esp),%ecx
		cld
		NOP
		rep
		insb
		NOP
		movl	%edi,%eax
		popl	%edi
		ret

# insw(port,addr,cnt)

ENTRY(insw)
		pushl	%edi
		movw	8(%esp),%dx
		movl	12(%esp),%edi
		movl	16(%esp),%ecx
		cld
		NOP
		.byte 	0x66,0xf2,0x6d	# rep insw
		NOP
		movl	%edi,%eax
		popl	%edi
		ret

# outsw(port,addr,cnt)

ENTRY(outsw)
		pushl	%esi
		movw	8(%esp),%dx
		movl	12(%esp),%esi
		movl	16(%esp),%ecx
		cld
		NOP
		.byte 	0x66,0xf2,0x6f	# rep outsw
		NOP
		movl	%esi,%eax
		popl	%esi
		ret

# outsb(port,addr,cnt)

ENTRY(outsb)
		pushl	%esi
		movw	8(%esp),%dx
		movl	12(%esp),%esi
		movl	16(%esp),%ecx
		cld
		NOP
		rep
		outsb
		NOP
		movl	%esi,%eax
		popl	%esi
		ret

/*
* void lgdt(struct region_descriptor *rdp);
*/

ENTRY(lgdt)
/* reload the descriptor table */
		movl	4(%esp),%eax
		lgdt	(%eax)
/* flush the prefetch q */
		jmp		1f
		nop
1:
/* reload "stale" selectors */
		# movw	$GSEL(GDATA_SEL, SEL_KPL),%ax
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%ss

/* reload code selector by turning return into intersegmental return */
		movl	0(%esp),%eax
		pushl	%eax
		# movl	$GSEL(GCODE_SEL, SEL_KPL),4(%esp)
		movl	$8,4(%esp)
		lret

/*
 * void lidt(struct region_descriptor *rdp);
 */

ENTRY(lidt)
		movl	4(%esp),%eax
		lidt	(%eax)
		ret

/*
 * void lldt(u_short sel)
 */

ENTRY(lldt)
		lldt	4(%esp)
		ret

/*
 * void ltr(u_short sel)
 */

ENTRY(ltr)
		ltr		4(%esp)
		ret

/*
 * void lcr3(caddr_t cr3)
 */
ENTRY(lcr3)
		inb		$0x84,%al	# check wristwatch
		movl	4(%esp),%eax
 		orl		$I386_CR3PAT,%eax
		movl	%eax,%cr3
		inb		$0x84,%al	# check wristwatch
		ret

# tlbflush()
ENTRY(tlbflush)
		inb		$0x84,%al	# check wristwatch
		movl	%cr3,%eax
 		orl		$I386_CR3PAT,%eax
		movl	%eax,%cr3
		inb		$0x84,%al	# check wristwatch
		ret

# lcr0(cr0)
ENTRY(lcr0)
		movl	4(%esp),%eax
		movl	%eax,%cr0
		ret

# rcr0()
ENTRY(rcr0)
		movl	%cr0,%eax
		ret

# rcr2()
ENTRY(rcr2)
		movl	%cr2,%eax
		ret

# rcr3()
ENTRY(cr3)
ENTRY(rcr3)
		movl	%cr3,%eax
		ret

/*
 * {fu,su},{byte,word}
 */

ALTENTRY(fuiword)
ENTRY(fuword)
		movl	_C_LABEL(curpcb),%ecx
		movl	$fusufault,PCB_ONFAULT(%ecx)
		movl	4(%esp),%edx
		.byte	0x65					# use gs
		movl	0(%edx),%eax
		movl	$0,PCB_ONFAULT(%ecx)
		ret


ENTRY(fusword)
		movl	_C_LABEL(curpcb),%ecx
		movl	$fusufault,PCB_ONFAULT(%ecx) #in case we page/protection violate
		movl	4(%esp),%edx
		.byte	0x65					# use gs
		movzwl	0(%edx),%eax
		movl	$0,PCB_ONFAULT(%ecx)
		ret


ALTENTRY(fuibyte)
ENTRY(fubyte)
		movl	_C_LABEL(curpcb),%ecx
		movl	$fusufault,PCB_ONFAULT(%ecx) #in case we page/protection violate
		movl	4(%esp),%edx
		.byte	0x65					# use gs
		movzbl	0(%edx),%eax
		movl	$0,PCB_ONFAULT(%ecx)
		ret


ENTRY(fusufault)
		movl	_C_LABEL(curpcb),%ecx
		xorl	%eax,%eax
		movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
		decl	%eax
		ret


ALTENTRY(suiword)
ENTRY(suword)
		movl	_C_LABEL(curpcb),%ecx
		movl	$fusufault,PCB_ONFAULT(%ecx) #in case we page/protection violate
		movl	4(%esp),%edx
		movl	8(%esp),%eax

#ifdef notdef
		shrl	$IDXSHIFT, %edx			/* fetch pte associated with address */
		andb	$0xfc, %dl
		movl	_C_LABEL(PTmap)(%edx), %edx

		andb	$7, %dl					/* if we are the one case that won't trap... */
		cmpb	$5 , %edx
		jne		1f
/* ... then simulate the trap! */
		pushl	%edi
		call	_C_LABEL(trapwrite)		/* trapwrite(addr) */
		popl	%edx
		cmpl	$0, %eax				/* if not ok, return */
		jne		fusufault
		movl	8(%esp),%eax			/* otherwise, continue with reference */
1:
		movl	4(%esp),%edx
#endif
		.byte	0x65					# use gs
		movl	%eax,0(%edx)
		xorl	%eax,%eax
		movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
		ret


ENTRY(susword)
		movl	_C_LABEL(curpcb),%ecx
		movl	$fusufault,PCB_ONFAULT(%ecx) #in case we page/protection violate
		movl	4(%esp),%edx
		movl	8(%esp),%eax
#ifdef notdef
		shrl	$IDXSHIFT, %edx			/* calculate pte address */
		andb	$0xfc, %dl
		movl	_C_LABEL(PTmap)(%edx), %edx
		andb	$7, %edx				/* if we are the one case that won't trap... */
		cmpb	$5 , %edx
		jne		1f
/* ..., then simulate the trap! */
		pushl	%edi
		call	_C_LABEL(trapwrite)		/* trapwrite(addr) */
		popl	%edx
		movl	_C_LABEL(curpcb), %ecx			# restore trashed registers
		cmpl	$0, %eax				/* if not ok, return */
		jne		fusufault
		movl	8(%esp),%eax
1: 		movl	4(%esp),%edx
#endif
		.byte	0x65					# use gs
		movw	%ax,0(%edx)
		xorl	%eax,%eax
		movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
		ret


ALTENTRY(suibyte)
ENTRY(subyte)
		movl	_C_LABEL(curpcb),%ecx
		movl	$fusufault,PCB_ONFAULT(%ecx) #in case we page/protection violate
		movl	4(%esp),%edx
		movl	8(%esp),%eax
#ifdef notdef
		shrl	$IDXSHIFT, %edx			/* calculate pte address */
		andb	$0xfc, %dl
		movl	_PTmap(%edx), %edx
		andb	$7, %edx				/* if we are the one case that won't trap... */
		cmpb	$5 , %edx
		jne		1f
/* ..., then simulate the trap! */
		pushl	%edi
		call	_C_LABEL(trapwrite)		/* trapwrite(addr) */
		popl	%edx
		movl	_C_LABEL(curpcb), %ecx			# restore trashed registers
		cmpl	$0, %eax				/* if not ok, return */
		jne		fusufault
		movl	8(%esp),%eax
1: 		movl	4(%esp),%edx
#endif
		.byte	0x65					# use gs
		movb	8(%esp),%al
		movb	%al,(%edx)
		xorl	%eax,%eax
		movl	%eax,PCB_ONFAULT(%ecx) #in case we page/protection violate
		ret

ENTRY(htonl)
ENTRY(ntohl)
		movl	4(%esp),%eax
		xchgb	%al,%ah
		roll	$16,%eax
		xchgb	%al,%ah
		ret

ENTRY(htons)
ENTRY(ntohs)
		movzwl	4(%esp),%eax
		xchgb	%al,%ah
		ret
		
/*****************************************************************************/
/* setjump, longjump                                                         */
/*****************************************************************************/

ENTRY(setjmp)
		movl	4(%esp),%eax
		movl	%ebx, 0(%eax)		# save ebx
		movl	%esp, 4(%eax)		# save esp
		movl	%ebp, 8(%eax)		# save ebp
		movl	%esi,12(%eax)		# save esi
		movl	%edi,16(%eax)		# save edi
		movl	(%esp),%edx			# get rta
		movl	%edx,20(%eax)		# save eip
		xorl	%eax,%eax			# return (0);
		ret

ENTRY(longjmp)
		movl	4(%esp),%eax
		movl	0(%eax),%ebx		# restore ebx
		movl	4(%eax),%esp		# restore esp
		movl	8(%eax),%ebp		# restore ebp
		movl	12(%eax),%esi		# restore esi
		movl	16(%eax),%edi		# restore edi
		movl	20(%eax),%edx		# get rta
		movl	%edx,(%esp)			# put in return frame
		xorl	%eax,%eax			# return (1);
		incl	%eax
		ret
