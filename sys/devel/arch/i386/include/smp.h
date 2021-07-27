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

#include <devel/arch/i386/isa/icu.h>

#include <i386/include/frame.h>
#include <i386/include/intr.h>
#include <i386/include/pcb.h>

#define LAPIC_IPI_INTS 	0xf0
#define	IPI_INVLOP		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, amd64 */
#define	IPI_INVLTLB		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, i386 */
#define	IPI_INVLPG		(LAPIC_IPI_INTS + 2)
#define	IPI_INVLRNG		(LAPIC_IPI_INTS + 3)
#define	IPI_INVLCACHE	(LAPIC_IPI_INTS + 4)

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

//		IDTVEC(ipi_intr_bitmap_handler), 	/* Bitmap based IPIs */
//		IDTVEC(ipi_swi),					/* Runs delayed SWI */
//		IDTVEC(cpustop),					/* CPU stops & waits to be restarted */
//		IDTVEC(cpususpend),					/* CPU suspends & waits to be resumed */
//		IDTVEC(rendezvous);					/* handle CPU rendezvous */

/*
void	invltlb_handler(void);
void	invlpg_handler(void);
void	invlrng_handler(void);
void	invlcache_handler(void);
*/
typedef void (*smp_invl_cb_t)(struct pmap *, vm_offset_t addr1, vm_offset_t addr2);

void 	smp_masked_invltlb(cpuset_t, pmap_t, smp_invl_cb_t);
void	smp_masked_invlpg(cpuset_t, vm_offset_t, pmap_t, smp_invl_cb_t);
void	smp_masked_invlpg_range(cpuset_t, vm_offset_t, vm_offset_t, pmap_t, smp_invl_cb_t);
void	smp_cache_flush(smp_invl_cb_t);

/* functions in mpboot.s */
void 	bootMP(void);

#endif /* _I386_SMP_H_ */
