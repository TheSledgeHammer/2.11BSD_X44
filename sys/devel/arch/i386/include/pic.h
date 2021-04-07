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

#ifndef _I386_PIC_H_
#define _I386_PIC_H_

#include <sys/queue.h>

/*
 * Methods that a PIC provides to mask/unmask a given interrupt source,
 * "turn on" the interrupt on the CPU side by setting up an IDT entry, and
 * return the vector associated with this source.
 */
struct pic {
	int 					pic_type;
	void					(*pic_hwmask)(struct ioapic_intsrc *, int);
	void 					(*pic_hwunmask)(struct ioapic_intsrc *, int);
	void 					(*pic_addroute)(struct ioapic_intsrc *, struct cpu_info *, int, int, int);
	void 					(*pic_delroute)(struct ioapic_intsrc *, struct cpu_info *, int, int, int);
	TAILQ_ENTRY(pic) 		pic_entry;
};

/*
 * PIC types.
 */
#define PIC_I8259			0
#define PIC_IOAPIC			1
#define PIC_LAPIC			2
#define PIC_SOFT			3

/*
 * An interrupt source.  The upper-layer code uses the PIC methods to
 * control a given source.  The lower-layer PIC drivers can store additional
 * private data in a given interrupt source such as an interrupt pin number
 * or an I/O APIC pointer.
 */
struct intsrc {
	struct pic 				*is_pic;
    struct intrhand     	*is_handlers;	/* handler chain */
	u_long 					*is_count;
	u_long 					*is_straycount;
	u_int 					is_index;
	u_int 					is_domain;
	u_int 					is_cpu;
};

/*
 * Interrupt handler chains.  isa_intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
    struct pic          	*ih_pic;
	int						(*ih_fun)(void *);
	void					*ih_arg;
	u_long					ih_count;
	struct intrhand 		*ih_next;
    struct intrhand 		*ih_prev;
	int						ih_level;
	int						ih_irq;
};

/*
 * Software definitions belonging to Local APIC driver.
 */

#ifdef _KERNEL
extern volatile vaddr_t local_apic_va;
extern boolean_t x2apic_mode;
#endif
/*
 * 'pin numbers' for local APIC
 */
#define LAPIC_PIN_TIMER		0
#define LAPIC_PIN_PCINT		2
#define LAPIC_PIN_LVINT0	3
#define LAPIC_PIN_LVINT1	4
#define LAPIC_PIN_LVERR		5

extern struct lock_object 	*icu_lock;

extern void 			lapic_boot_init(paddr_t);
extern void 			lapic_set_lvt(void);
extern void 			lapic_enable(void);
extern void 			lapic_calibrate_timer(boolean_t);
extern void 			lapic_reset(void);

extern uint32_t 		lapic_read(u_int);
extern void 			lapic_write(u_int, uint32_t);
extern void 			lapic_write_tpri(uint32_t);
extern uint32_t 		lapic_cpu_number(void);
extern boolean_t 		lapic_is_x2apic(void);

#endif /* _I386_PIC_H_ */
