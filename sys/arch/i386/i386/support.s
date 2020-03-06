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

#include <machine/asmacros.h>
#include <machine/cputypes.h>
#include <machine/pmap.h>
#include <machine/specialreg.h>

#include "assym.s"

#define	NOP	inb $0x84, %al ; inb $0x84, %al
#define	ALIGN32	.align 2	/* 2^2  = 4 */

/*
 * Support routines for GCC
 */

ENTRY(udivsi3)
		movl 4(%esp),%eax
		xorl %edx,%edx
		divl 8(%esp)
		ret
END(udivsi3)

ENTRY(divsi3)
		movl 4(%esp),%eax
		#xorl %edx,%edx		/* not needed - cltd sign extends into %edx */
		cltd
		idivl 8(%esp)
		ret
END(divsi3)

/*
* I/O bus instructions via C
*/

ENTRY(inb)
		movl	4(%esp),%edx
		subl	%eax,%eax		# clr eax
		NOP
		inb		%dx,%al
		ret
END(inb)

ENTRY(inw)
		movl	4(%esp),%edx
		subl	%eax,%eax		# clr eax
		NOP
		inw		%dx,%ax
		ret
END(inw)

ENTRY(rtcin)
		movl	4(%esp),%eax
		outb	%al,$0x70
		subl	%eax,%eax		# clr eax
		inb		$0x71,%al		# Compaq SystemPro
		ret
END(rtcin)

ENTRY(outb)
		movl	4(%esp),%edx
		NOP
		movl	8(%esp),%eax
		outb	%al,%dx
		NOP
		ret
END(outb)

ENTRY(outw)
		movl	4(%esp),%edx
		NOP
		movl	8(%esp),%eax
		outw	%ax,%dx
		NOP
		ret
END(outw)

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
END(bzero)

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
END(fillw)

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
END(bcopyb)

/*
 * (ov)bcopy (src,dst,cnt)
 *  ws@tools.de     (Wolfgang Solfrank, TooLs GmbH) +49-228-985800
 */

ENTRY(ovbcopy)
ENTRY(bcopy)
		pushl	%esi
		pushl	%edi
		movl	12(%esp),%esi
		movl	16(%esp),%edi
		movl	20(%esp),%ecx
		cmpl	%esi,%edi	/* potentially overlapping? */
		jnb		1f
		cld					/* nope, copy forwards. */
		shrl	$2,%ecx		/* copy by words */
		rep
		movsl
		movl	20(%esp),%ecx
		andl	$3,%ecx		/* any bytes left? */
		rep
		movsb
		popl	%edi
		popl	%esi
		xorl	%eax,%eax
		ret
		ALIGN32
1:
		addl	%ecx,%edi	/* copy backwards. */
		addl	%ecx,%esi
		std
		andl	$3,%ecx		/* any fractional bytes? */
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
END(bcopy)
END(ovbcopy)

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
END(insb)

# insw(port,addr,cnt)

ENTRY(insw)
		pushl	%edi
		movw	8(%esp),%dx
		movl	12(%esp),%edi
		movl	16(%esp),%ecx
		cld
		NOP
		.byte 0x66,0xf2,0x6d	# rep insw
		NOP
		movl	%edi,%eax
		popl	%edi
		ret
END(insw)

# outsw(port,addr,cnt)

ENTRY(outsw)
		pushl	%esi
		movw	8(%esp),%dx
		movl	12(%esp),%esi
		movl	16(%esp),%ecx
		cld
		NOP
		.byte 0x66,0xf2,0x6f	# rep outsw
		NOP
		movl	%esi,%eax
		popl	%esi
		ret
END(outsw)

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
END(outsb)

/*
* void lgdt(struct region_descriptor *rdp);
*/

ENTRY(lgdt)
		/* reload the descriptor table */
		movl	4(%esp),%eax
		lgdt	(%eax)
		/* flush the prefetch q */
		jmp	1f
		nop
1:
		/* reload "stale" selectors */
		# movw	$KDSEL,%ax
		movw	$0x10,%ax
		movw	%ax,%ds
		movw	%ax,%es
		movw	%ax,%ss

		/* reload code selector by turning return into intersegmental return */
		movl	0(%esp),%eax
		pushl	%eax
		# movl	$KCSEL,4(%esp)
		movl	$8,4(%esp)
		lret
END(lgdt)

/*
 * void lidt(struct region_descriptor *rdp);
 */

ENTRY(lidt)
		movl	4(%esp),%eax
		lidt	(%eax)
		ret
END(lidt)

/*
 * void lldt(u_short sel)
 */

ENTRY(lldt)
		lldt	4(%esp)
		ret
END(lldt)

/*
 * void ltr(u_short sel)
 */

ENTRY(ltr)
		ltr	4(%esp)
		ret
END(ltr)


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
END(setjmp)

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
END(longjmp)
