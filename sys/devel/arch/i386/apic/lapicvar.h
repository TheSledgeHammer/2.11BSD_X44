/*	$NetBSD: i82489var.h,v 1.21 2020/05/21 21:12:30 ad Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

#ifndef _I386_LAPICVAR_H_
#define _I386_LAPICVAR_H_
/*
 * Software definitions belonging to Local APIC driver.
 */

#ifdef _KERNEL
extern volatile u_int32_t  local_apic_va;
extern boolean_t x2apic_mode;
#endif

#define ioapic_asm_ack(num) \
	movl	$0,local_apic_va + LAPIC_EOI

/*
 * "spurious interrupt vector"; vector used by interrupt which was
 * aborted because the CPU masked it after it happened but before it
 * was delivered.. "Oh, sorry, i caught you at a bad time".
 * Low-order 4 bits must be all ones.
 */
extern void Xintrspurious(void);
#define LAPIC_SPURIOUS_VECTOR		0xef

/*
 * Vectors used for inter-processor interrupts.
 */
extern void Xintr_lapic_ipi(void);
extern void Xintr_x2apic_ipi(void);
extern void Xrecurse_lapic_ipi(void);
extern void Xresume_lapic_ipi(void);
#define LAPIC_IPI_VECTOR			0xe0

extern void Xintr_lapic_tlb(void);
extern void Xintr_x2apic_tlb(void);
#define LAPIC_TLB_VECTOR			0xe1

/*
 * Vector used for local apic timer interrupts.
 */

extern void Xintr_lapic_ltimer(void);
extern void Xintr_x2apic_ltimer(void);
extern void Xresume_lapic_ltimer(void);
extern void Xrecurse_lapic_ltimer(void);
#define LAPIC_TIMER_VECTOR			0xc0

/*
 * 'pin numbers' for local APIC
 */
#define LAPIC_PIN_TIMER				0
#define LAPIC_PIN_PCINT				2
#define LAPIC_PIN_LVINT0			3
#define LAPIC_PIN_LVINT1			4
#define LAPIC_PIN_LVERR				5

struct cpu_info;

extern void 			lapic_boot_init(caddr_t);
extern void 			lapic_set_lvt(void);
extern void 			lapic_enable(void);
extern void 			lapic_calibrate_timer(boolean_t);
extern void 			lapic_reset(void);

extern uint32_t 		lapic_read(u_int);
extern void 			lapic_write(u_int, uint32_t);
extern void 			lapic_write_tpri(uint32_t);
extern uint32_t 		lapic_cpu_number(void);
extern boolean_t 		lapic_is_x2apic(void);

#endif /* _I386_LAPICVAR_H_ */
