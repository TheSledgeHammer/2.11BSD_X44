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
 */

#ifndef _I386_IOAPICVAR_H_
#define _I386_IOAPICVAR_H_

#include <sys/queue.h>

struct ioapic_head;
SIMPLEQ_HEAD(ioapic_head, ioapic_softc);
struct ioapic_softc {
	SIMPLEQ_ENTRY(ioapic_softc) sc_next;
	struct softpic				*sc_softpic;
	struct pic 					*sc_pic;
	struct device				*sc_dev;
	int							sc_apicid;
	int							sc_apic_vers;
	int							sc_apic_vecbase; 	/* global int base if ACPI */
	int							sc_apic_sz;			/* apic size*/
	int							sc_flags;
	caddr_t						sc_pa;				/* PA of ioapic */
	volatile u_int32_t			*sc_reg;			/* KVA of ioapic addr */
	volatile u_int32_t			*sc_data;			/* KVA of ioapic data */
};

/*
 * MP: intr_handle_t is bitfielded.
 * ih&0xff -> legacy irq number.
 * ih&0x10000000 -> if 0, old-style isa irq; if 1, routed via ioapic.
 * (ih&0xff0000)>>16 -> ioapic id.
 * (ih&0x00ff00)>>8 -> ioapic pin.
 *
 * MSI/MSI-X:
 * (ih&0x000ff80000000000)>>43 -> MSI/MSI-X device id.
 * (ih&0x000007ff00000000)>>32 -> MSI/MSI-X vector id in a device.
 */
#define APIC_INT_VIA_APIC		0x10000000
#define APIC_INT_VIA_MSG		0x20000000
#define APIC_INT_APIC_MASK		0x00ff0000
#define APIC_INT_APIC_SHIFT		16
#define APIC_INT_PIN_MASK		0x0000ff00
#define APIC_INT_PIN_SHIFT		8
#define APIC_INT_LINE_MASK		0x000000ff

#define APIC_IRQ_APIC(x) 		((x & APIC_INT_APIC_MASK) >> APIC_INT_APIC_SHIFT)
#define APIC_IRQ_PIN(x) 		((x & APIC_INT_PIN_MASK) >> APIC_INT_PIN_SHIFT)
#define APIC_IRQ_ISLEGACY(x) 	(bool)(!((x) & APIC_INT_VIA_APIC))
#define APIC_IRQ_LEGACY_IRQ(x) 	(int)((x) & 0xff)

void ioapic_print_redir(struct ioapic_softc *, const char *, int);
struct ioapic_softc 	*ioapic_find(int);
struct ioapic_softc 	*ioapic_find_bybase(int);

extern int nioapics;
extern struct ioapic_head ioapics;

#endif /* _I386_IOAPICVAR_H_ */
