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

#include <machine/softpic.h>

/*
 * APIC extends the PIC methods, providing mapping register of
 * the apic, x2apic and legacy interrupt sources.
 */
struct apic {
    int                     apic_pic_type;
    struct intrstub         *apic_edge;
    struct intrstub         *apic_level;
    void 					*apic_resume;
    void 					*apic_recurse;
    TAILQ_ENTRY(apic)       apic_entry;
};

/*
 * Methods that a PIC provides to mask/unmask a given interrupt source,
 * "turn on" the interrupt on the CPU side by setting up an IDT entry, and
 * return the vector associated with this source.
 */
struct pic {
	int 					pic_type;
	void					(*pic_hwmask)(struct softpic *, int);
	void 					(*pic_hwunmask)(struct softpic *, int);
	void 					(*pic_addroute)(struct softpic *, struct cpu_info *, int, int, int);
	void 					(*pic_delroute)(struct softpic *, struct cpu_info *, int, int, int);
	void					(*pic_register)(struct pic *, struct apic *);
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
struct intrsource {
	struct pic 				*is_pic;
	struct apic 			*is_apic;
    struct intrhand     	*is_handlers;	/* handler chain */
	u_long 					*is_count;
	//u_long 					*is_straycount;
	u_int 					is_index;
	int						is_type;
	int 					is_pin;
	int  					is_minlevel;
	int 					is_maxlevel;
};

/*
 * Interrupt handler chains.  isa_intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
    struct pic          	*ih_pic;
    struct apic 			*ih_apic;
	int						(*ih_fun)(void *);
	void					*ih_arg;
	u_long					ih_count;
	struct intrhand 		*ih_next;
    struct intrhand 		*ih_prev;
	int						ih_level;
	int						ih_irq;
	int 					ih_slot;
};

/*
 * Struct describing an interrupt source for a CPU. struct cpu_info
 * has an array of MAX_INTR_SOURCES of these. The index in the array
 * is equal to the stub number of the stubcode as present in vector.s
 *
 * The primary CPU's array of interrupt sources has its first 16
 * entries reserved for legacy ISA irq handlers. This means that
 * they have a 1:1 mapping for arrayindex:irq_num. This is not
 * true for interrupts that come in through IO APICs, to find
 * their source, go through ci->ci_isources[index].is_pic
 *
 * It's possible to always maintain a 1:1 mapping, but that means
 * limiting the total number of interrupt sources to MAX_INTR_SOURCES
 * (32), instead of 32 per CPU. It also would mean that having multiple
 * IO APICs which deliver interrupts from an equal pin number would
 * overlap if they were to be sent to the same CPU.
 */
struct intrstub {
	void 					*ist_entry;
	void 					*ist_recurse;
	void 					*ist_resume;
};

struct pcibus_attach_args;

/* intrstubs */
extern struct intrstub		legacy_stubs[];
extern struct intrstub		apic_edge_stubs[];
extern struct intrstub		apic_level_stubs[];
extern struct intrstub		x2apic_edge_stubs[];
extern struct intrstub		x2apic_level_stubs[];

/* pic templates */
extern struct pic			i8259_template;
extern struct pic			ioapic_template;
extern struct pic			lapic_template;
extern struct pic			softintr_template;

/* apic intrmaps */
extern struct apic          	i8259_intrmap;
extern struct apic          	ioapic_intrmap;
extern struct apic          	lapic_intrmap;
extern struct apic 	 	softintr_intrmap;

/* intr.c */
void 			*intr_establish(int, int, int, int (*)(void *), void *, bool_t, int);
void			intr_disestablish(int, bool_t, int);
void			fakeintr(struct softpic *, struct intrhand *, u_int);
void			intr_default_setup(void);
void			intr_calculatemasks(void);
void 			intr_add_pcibus(struct pcibus_attach_args *);
int	 			intr_find_mpmapping(int, int, int *);
void			apic_intr_string(char *, void *, int);
char 			*intr_typename(int);
#endif /* _I386_PIC_H_ */
