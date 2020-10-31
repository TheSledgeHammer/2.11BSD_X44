/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 * $FreeBSD$
 *
 *		originally from: locore.s, by William F. Jolitz
 *
 *		Substantially rewritten by David Greenman, Rod Grimes,
 *			Bruce Evans, Wolfgang Solfrank, Poul-Henning Kamp
 *			and many others.
 */

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
		movl	$CPU_386,cpu
		jmp		3f

trynexgen:
		popfl
		movl	$CPU_NX586,cpu
		movl	$0x4778654e,cpu_vendor			# store vendor string
		movl	$0x72446e65,cpu_vendor+4
		movl	$0x6e657669,cpu_vendor+8
		movl	$0,cpu_vendor+12
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
		movl	$CPU_486,cpu

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
		movl	$0x69727943,cpu_vendor		# store vendor string
		movl	$0x736e4978,cpu_vendor+4
		movl	$0x64616574,cpu_vendor+8
		jmp		3f

trycpuid:	/* Use the `cpuid' instruction. */
		xorl	%eax,%eax
		cpuid								# cpuid 0
		movl	%eax,cpu_high				# highest capability
		movl	%ebx,cpu_vendor				# store vendor string
		movl	%edx,cpu_vendor+4
		movl	%ecx,cpu_vendor+8
		movb	$0,cpu_vendor+12

		movl	$1,%eax
		cpuid								# cpuid 1
		movl	%eax,cpu_id					# store cpu_id
		movl	%ebx,cpu_procinfo			# store cpu_procinfo
		movl	%edx,cpu_feature			# store cpu_feature
		movl	%ecx,cpu_feature2			# store cpu_feature2
		rorl	$8,%eax						# extract family type
		andl	$15,%eax
		cmpl	$5,%eax
		jae		1f

		/* less than Pentium; must be 486 */
		movl	$CPU_486,cpu
		jmp		3f
1:
		/* a Pentium? */
		cmpl	$5,%eax
		jne		2f
		movl	$CPU_586,cpu
		jmp		3f
2:
		/* Greater than Pentium...call it a Pentium Pro */
		movl	$CPU_686,cpu
3:
		popl	%ebx
		ret
