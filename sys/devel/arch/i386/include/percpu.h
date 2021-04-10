/*
 * percpu.h
 *
 *  Created on: 10 Apr 2021
 *      Author: marti
 */

#ifndef _MACHINE_PERCPU_H_
#define _MACHINE_PERCPU_H_

extern struct percpu 				__percpu[];

#define	PERCPU_MD_FIELDS 														\
	struct	cpu_info				*pc_cpuinfo;		/* Self-reference */	\
	struct	segment_descriptor 		pc_common_tssd;								\
	struct	segment_descriptor 		*pc_tss_gdt;								\
	struct	segment_descriptor 		*pc_fsgs_gdt;								\
	struct	i386tss 				*pc_common_tssp;							\
	u_int							pc_trampstk;								\
	int								pc_currentldt;								\
	u_int   						pc_acpi_id;			/* ACPI CPU id */		\
	u_int							pc_apic_id;			/* APIC CPU id */		\
	char							*pc_copyout_buf;							\

#ifdef _KERNEL
#define	PERCPU_PTR(pc, name)		((pc)->pc_##name)
#define PERCPU_SET(pc, name, val)	(PERCPU_PTR(pc, name) = (val))
#define PERCPU_GET(pc, name)        (PERCPU_PTR(pc, name))
#define PERCPU_ADD(pc, name, val)   (PERCPU_PTR(pc, name) += (val))

#define	IS_BSP(pc)					(PCPU_GET(pc, cpuid) == 0)

struct kthread *
__curkthread(void)
{
	struct kthread *kt;
	__asm("movl %%fs:%1,%0" : "=r" (kt)
			: "m" (*(char *)offsetof(struct percpu, pc_curkthread)));
	return (kt);
}

#endif /* _KERNEL */

#endif /* _MACHINE_PERCPU_H_ */
