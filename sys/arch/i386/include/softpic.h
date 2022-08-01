/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _I386_SOFTPIC_H_
#define _I386_SOFTPIC_H_

/*
 * Softpic_pin, serves as the selector pin, or equivalent to ioapic_pin in NetBSD
 */
struct softpic_pin {
	struct softpic			*sp_next;
    unsigned int 			sp_vector:8;
    int                 	sp_irq;
    int                 	sp_pin;
    int                 	sp_apicid;
    int						sp_type;
    struct cpu_info     	*sp_cpu;
    struct mp_intr_map 		*sp_map;
};

/*
 * Softpic, acts as a selector for which PIC/APIC to use: see softpic.c
 */
struct softpic {
    struct intrsource       *sp_intsrc;
    struct intrhand         *sp_inthnd;
    struct ioapic_softc		*sp_ioapic;
    struct softpic_pin		sp_pins[0];
    int                     sp_template;
    bool_t               	sp_isapic;

    int 					sp_idtvec;
    int 					sp_slot;
};

struct cpu_info;
struct apic;
struct pic;

extern struct lock_object 	icu_lock;
extern int 					intr_shared_edge;		/* This system has shared edge interrupts */
extern struct softpic		*intrspic;
extern struct intrsource 	*intrsrc[];
extern struct intrhand 		*intrhand[];

/* softpic.c */
void 			softpic_init(void);
void			softpic_register(struct pic *, struct apic *);
struct apic		*softpic_handle_apic(struct softpic *);
struct pic 		*softpic_handle_pic(struct softpic *);
void			softpic_pic_hwmask(struct softpic *, int, bool_t, int);
void			softpic_pic_hwunmask(struct softpic *, int, bool_t, int);
void			softpic_pic_addroute(struct softpic *, struct cpu_info *, int, int, int, bool_t, int);
void			softpic_pic_delroute(struct softpic *, struct cpu_info *, int, int, int, bool_t, int);
struct softpic 	*softpic_intr_handler(struct softpic *, int, int, bool_t, int);

#endif /* _I386_SOFTPIC_H_ */
