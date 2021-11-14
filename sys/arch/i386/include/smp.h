/*-
 * SPDX-License-Identifier: Beerware
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.org> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $FreeBSD$
 *
 */

#ifndef _I386_SMP_H_
#define _I386_SMP_H_

#include <sys/bus.h>

#include <arch/i386/isa/icu.h>

#include <machine/frame.h>
#include <machine/intr.h>
#include <machine/pcb.h>

#define LAPIC_IPI_INTS 	0xf0
#define	IPI_INVLOP		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, amd64 */
#define	IPI_INVLTLB		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, i386 */
#define	IPI_INVLPG		(LAPIC_IPI_INTS + 2)
#define	IPI_INVLRNG		(LAPIC_IPI_INTS + 3)

struct pmap;

/* global data in mp.c */
extern int	 			mp_naps;
extern int 				boot_cpu_id;
extern int 				cpu_apic_ids[];
extern int 				bootAP;
extern char 			*bootSTK;
extern void 			*bootstacks[];
extern unsigned int 	boot_address;
extern unsigned int 	bootMP_size;
extern int 				cpu_logical;
extern int 				cpu_cores;

/* SMP data in cpu.c */
extern u_int 			mp_maxid;
extern int 				mp_maxcpus;
extern int 				mp_ncores;
extern int 				mp_ncpus;
extern int 				smp_cpus;
extern volatile int 	smp_started;
extern int 				smp_threads_per_core;
extern u_int 			all_cpus;

/* IPI handlers (FreeBSD) */
#define	IDTVEC(name)	__CONCAT(X, name)
extern	IDTVEC(invltlb),					/* TLB shootdowns - global */
		IDTVEC(invlpg),						/* TLB shootdowns - 1 page */
		IDTVEC(invlrng),					/* TLB shootdowns - page range */
		IDTVEC(invlcache);					/* Write back and invalidate cache */

void 	bootMP(void);

void 	cpu_alloc(struct cpu_info *);

void	pmap_tlb_shootdown(pmap_t, vm_offset_t, vm_offset_t, int32_t *);
void	pmap_tlb_shootnow(pmap_t, int32_t);
void	pmap_do_tlb_shootdown(pmap_t, struct cpu_info *);

void 	smp_masked_invltlb(u_int, pmap_t);
void	smp_masked_invlpg(u_int, vm_offset_t, pmap_t);
void	smp_masked_invlpg_range(u_int, vm_offset_t, vm_offset_t, pmap_t);

#endif /* _I386_SMP_H_ */
