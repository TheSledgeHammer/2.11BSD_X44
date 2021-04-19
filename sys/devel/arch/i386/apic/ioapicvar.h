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

struct ioapic_intsrc {
	int 					io_irq;
	/*
	u_int 					io_intpin:8;
	u_int 					io_cpu;
	u_int 					io_activehi:1;
	u_int 					io_edgetrigger:1;
	u_int 					io_masked:1;
	int 					io_bus:4;
	uint32_t 				io_lowreg;
	u_int 					io_remap_cookie;
	*/

	struct intrsource		io_intsrc;
	struct mp_intr_map 		*io_map;
	u_int 					io_vector:8;
	int						io_type;
	struct cpu_info			*io_cpuinfo;
};

struct ioapic_head;
SIMPLEQ_HEAD(ioapic_head, ioapic_softc);
struct ioapic_softc {
	SIMPLEQ_ENTRY(ioapic) 	sc_next;
	struct ioapic_intsrc 	sc_pins;
	struct pic 				sc_pic;
	struct device			sc_dev;
	int						sc_apicid;
	int						sc_apic_vers;
	int						sc_apic_vecbase; 	/* global int base if ACPI */
	int						sc_apic_sz;			/* apic size*/
	int						sc_flags;
	caddr_t					sc_pa;				/* PA of ioapic */
	volatile u_int32_t		*sc_reg;			/* KVA of ioapic addr */
	volatile u_int32_t		*sc_data;			/* KVA of ioapic data */
};

struct ioapic_softc *ioapic_find(int);
struct ioapic_softc *ioapic_find_bybase(int);

#endif /* _I386_IOAPICVAR_H_ */
