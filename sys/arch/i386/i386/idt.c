/*	$NetBSD: idt.c,v 1.16 2022/02/13 19:21:21 riastradh Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility NASA Ames Research Center, and by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/atomic.h>

#include <machine/cpu.h>
#include <machine/segments.h>
#include <machine/intr.h>

char idt_allocmap[NIDT];

/*
 * IDT APIC Vectors:
 * highest irq = 16
 * highest level = 14
 * legacy vectors located between 225 to 256
 * note: level can go higher than 14, but vector is capped at 224;
 * Once beyond apic level 14.
 */
/*
 * Allocate an IDT vector slot within the given range.
 * cpu_lock will be held unless single threaded during early boot.
 */
int
idt_vec_alloc(low, high)
	int low, high;
{
    int vec;
    char *allocmap = idt_allocmap;

    if (low < 0 || high >= nitems(idt_allocmap)) {
        return (-1);
    }
    for (vec = low; vec <= high; vec++) {
        allocmap[vec] = idt_allocmap[vec];
        if (atomic_load_acquire(&allocmap[vec]) == 0) {
        	atomic_store_relaxed(&allocmap[vec], 1);
            return (vec);
        }
    }
    return (-1);
}

void
idt_vec_reserve(vec)
	int vec;
{
	int result;

	result = idt_vec_alloc(vec, vec);
	if (result < 0) {
		panic("%s: failed to reserve vec %d", __func__, vec);
	}
}

void
idt_vec_set(vec, function)
	int vec;
	void (*function)(void);
{
	char *allocmap = idt_allocmap;

	KASSERT(atomic_load_relaxed(&allocmap[vec]) == 1);

	setidt(vec, function, 0, SDT_SYS386IGT, SEL_KPL);
}

/*
 * Free IDT vector.  No locking required as release is atomic.
 */
void
idt_vec_free(vec)
	int vec;
{
	char *allocmap = idt_allocmap;

	KASSERT(atomic_load_relaxed(&allocmap[vec]) == 1);

	unsetidt(vec);
	atomic_store_release(&allocmap[vec], 0);
}

int
idtvector(irq, level, minlevel, maxlevel, isapic)
    int irq, level, minlevel, maxlevel;
    bool isapic;
{
    int idtvec;
    if(isapic) {
#ifdef IOAPIC_HWMASK
    	if (level > maxlevel) {
#else
    	if (minlevel == 0 || level < minlevel) {
#endif
    		 idtvec = idt_vec_alloc(APIC_LEVEL(level), IDT_INTR_HIGH);
    	}
    } else {
        idtvec = ICU_OFFSET + irq;
    }
    return (idtvec);
}
