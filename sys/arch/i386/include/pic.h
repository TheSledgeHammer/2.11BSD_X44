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

/* Softpic, acts as a selector for which PIC/APIC to use: see softpic.c */
struct softpic {
    struct cpu_info         *sp_cpu;
    struct intrsource       *sp_intsrc;
    struct intrhand         *sp_inthnd;
    int                     sp_template;
    unsigned int 			sp_vector:8;
    int                     sp_irq;
    int                     sp_pin;
    int                     sp_apicid;
    int						sp_type;
    bool_t               	sp_isapic;
    struct mp_intr_map 		*sp_map;
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
	void					(*pic_register)(void *);
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
    	struct intrhand     	*is_handlers;	/* handler chain */
	u_long 					*is_count;
	u_long 					*is_straycount;
	u_int 					is_index;
	u_int 					is_domain;
	u_int 					is_cpu;
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
	int						(*ih_fun)(void *);
	void					*ih_arg;
	u_long					ih_count;
	struct intrhand 		*ih_next;
    struct intrhand 		*ih_prev;
	int						ih_level;
	int						ih_irq;
};

struct pcibus_attach_args;
struct cpu_info;

extern struct lock_object 	*icu_lock;
extern int 					intr_shared_edge;		/* This system has shared edge interrupts */
extern struct softpic		*intrspic;

/* softpic.c */
void			softpic_pic_init(void *);
int				softpic_register_pic(struct pic *);
struct pic 		*softpic_handle_pic(struct softpic *);
void			softpic_pic_hwmask(struct softpic *, int, bool_t, int);
void			softpic_pic_hwunmask(struct softpic *, int, bool_t, int);
void			softpic_pic_addroute(struct softpic *, struct cpu_info *, int, int, int, bool_t, int);
void			softpic_pic_delroute(struct softpic *, struct cpu_info *, int, int, int, bool_t, int);
struct softpic 	*softpic_intr_handler(struct softpic *, int, int, bool_t, int);

/* intr.c */
void 			*intr_establish(bool_t, int, int, int, int, int (*)(void *), void *);
void			intr_disestablish(struct intrhand *);
int				fakeintr(void *);

void			intr_default_setup(void);
void			intr_calculatemasks(void);

void 			intr_add_pcibus(struct pcibus_attach_args *);
int	 			intr_find_mpmapping(int, int, int *);

/* apic_machdep.c */
void 			*apic_intr_establish(int, int, int, int (*)(void *), void *);
void			apic_intr_disestablish(void *);
#endif /* _I386_PIC_H_ */
