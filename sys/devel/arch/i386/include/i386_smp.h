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

#include <i386/include/apicvar.h>

#include <machine/frame.h>
#include <machine/intr_machdep.h>
#include <machine/pcb.h>

struct pmap;

/* global data in mp_i386.c */
extern int mp_naps;
extern int boot_cpu_id;
extern int cpu_apic_ids[];
extern int bootAP;
extern char *bootSTK;
extern void *bootstacks[];
extern unsigned int boot_address;
extern unsigned int bootMP_size;
extern int cpu_logical;
extern int cpu_cores;

/* IPI handlers */
#define	IDTVEC(name)	__CONCAT(X, name)
extern	IDTVEC(ipi_intr_bitmap_handler), 	/* Bitmap based IPIs */
		IDTVEC(ipi_swi),					/* Runs delayed SWI */
		IDTVEC(cpustop),					/* CPU stops & waits to be restarted */
		IDTVEC(cpususpend),					/* CPU suspends & waits to be resumed */
		IDTVEC(rendezvous);					/* handle CPU rendezvous */

#endif /* _I386_SMP_H_ */
