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

#ifndef _LOCORE

#define LAPIC_IPI_INTS 	0xf0
#define	IPI_INVLOP		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, amd64 */
#define	IPI_INVLTLB		(LAPIC_IPI_INTS + 1)	/* TLB Shootdown IPIs, i386 */
#define	IPI_INVLPG		(LAPIC_IPI_INTS + 2)
#define	IPI_INVLRNG		(LAPIC_IPI_INTS + 3)

/* global data in mp_machdep.c */
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

void 	bootMP(void);
void	alloc_ap_trampoline(int *, u_int64_t, u_int64_t);

extern void	invltlb_handler(void);
extern void	invlpg_handler(void);
extern void	invlrng_handler(void);
#endif
#endif /* _I386_SMP_H_ */
