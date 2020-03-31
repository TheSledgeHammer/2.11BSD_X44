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
 *	@(#)vector.s	1.0 (Berkeley) 03/31/20
 */

#include <i386/isa/icu.h>
#include <i386/isa/isa.h>

/* Interrupt vector table: */
IDTVEC(irq0)
	INTR1(0, IO_ICU1, IRQ0) ; INTREXIT1

IDTVEC(irq1)
	INTR1(1, IO_ICU1, IRQ1) ; INTREXIT1

IDTVEC(irq2)
	INTR1(1, IO_ICU1, IRQ2) ; INTREXIT1

IDTVEC(irq3)
	INTR1(3, IO_ICU1, IRQ3) ; INTREXIT1

IDTVEC(irq4)
	INTR1(4, IO_ICU1, IRQ4) ; INTREXIT1

IDTVEC(irq5)
	INTR1(5, IO_ICU1, IRQ5) ; INTREXIT1

IDTVEC(irq6)
	INTR1(6, IO_ICU1, IRQ6) ; INTREXIT1

IDTVEC(irq7)
	INTR1(7, IO_ICU1, IRQ7) ; INTREXIT1

IDTVEC(irq8)
	INTR2(8, IO_ICU2, IRQ8) ; INTREXIT2

IDTVEC(irq9)
	INTR2(9, IO_ICU2, IRQ9) ; INTREXIT2

IDTVEC(irq10)
	INTR2(10, IO_ICU2, IRQ10) ; INTREXIT2

IDTVEC(irq11)
	INTR2(11, IO_ICU2, IRQ11) ; INTREXIT2

IDTVEC(irq12)
	INTR2(12, IO_ICU2, IRQ12) ; INTREXIT2

IDTVEC(irq13)
	INTR2(13, IO_ICU2, IRQ13) ; INTREXIT2

IDTVEC(irq14)
	INTR2(14, IO_ICU2, IRQ14) ; INTREXIT2

IDTVEC(irq15)
	INTR2(15, IO_ICU2, IRQ15) ; INTREXIT2
