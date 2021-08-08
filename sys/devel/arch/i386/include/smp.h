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

#include <i386/include/frame.h>
#include <i386/include/intr.h>
#include <i386/include/pcb.h>

#define LAPIC_IPI_INTS 	0xf0
#define	IPI_INVLOP		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, amd64 */
#define	IPI_INVLTLB		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, i386 */
#define	IPI_INVLPG		(LAPIC_IPI_INTS + 2)
#define	IPI_INVLRNG		(LAPIC_IPI_INTS + 3)
//#define	IPI_INVLCACHE	(LAPIC_IPI_INTS + 4)

struct pmap;

/* global data in mp.c */
extern int	 		mp_naps;
extern int 			boot_cpu_id;
extern int 			cpu_apic_ids[];
extern int 			bootAP;
extern char 		*bootSTK;
extern void 		*bootstacks[];
extern unsigned int boot_address;
extern unsigned int bootMP_size;
extern int 			cpu_logical;
extern int 			cpu_cores;

struct lock_object smp_tlb_lock;

/* IPI handlers (FreeBSD) */
#define	IDTVEC(name)	__CONCAT(X, name)
extern	IDTVEC(invltlb),					/* TLB shootdowns - global */
		IDTVEC(invlpg),						/* TLB shootdowns - 1 page */
		IDTVEC(invlrng),					/* TLB shootdowns - page range */
		IDTVEC(invlcache);					/* Write back and invalidate cache */

void 	smp_masked_invltlb(u_int, pmap_t);
void	smp_masked_invlpg(u_int, vm_offset_t, pmap_t);
void	smp_masked_invlpg_range(u_int, vm_offset_t, vm_offset_t, pmap_t);
void	smp_cache_flush(smp_invl_cb_t);

/* functions in mpboot.s */
void 	bootMP(void);

/* intr.h */
#define I386_IPI_HALT			0x00000001
#define I386_IPI_MICROSET		0x00000002
#define I386_IPI_FLUSH_FPU		0x00000004
#define I386_IPI_SYNCH_FPU		0x00000008
#define I386_IPI_TLB			0x00000010
#define I386_IPI_MTRR			0x00000020
#define I386_IPI_GDT			0x00000040

/* pmap.h */
void	pmap_tlb_shootdown(pmap_t, vaddr_t, pt_entry_t, int32_t *);
void	pmap_tlb_shootnow(int32_t);
void	pmap_do_tlb_shootdown(struct cpu_info *);

#endif /* _I386_SMP_H_ */
