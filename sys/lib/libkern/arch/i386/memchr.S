/*
 * Written by J.T. Conklin <jtc@NetBSD.org>.
 * Public domain.
 */

#include <machine/asm.h>

#if defined(LIBC_SCCS)
	RCSID("$NetBSD: memchr.S,v 1.5 2003/12/04 13:57:31 keihan Exp $")
#endif

ENTRY(memchr)
	pushl	%edi
	movl	8(%esp),%edi		/* string address */
	movl	12(%esp),%eax		/* set character to search for */
	movl	16(%esp),%ecx		/* set length of search */
	testl	%ecx,%ecx			/* test for len == 0 */
	jz		L1
	cld							/* set search forward */
	repne						/* search! */
	scasb
	jne		L1					/* scan failed, return null */
	leal	-1(%edi),%eax		/* adjust result of scan */
	popl	%edi
	ret
	_ALIGN_TEXT,0x90
L1:	xorl	%eax,%eax
	popl	%edi
	ret
