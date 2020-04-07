/*
 * sysinit.h
 *
 *  Created on: 29 Mar 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_SYSINIT_H_
#define SYS_DEVEL_SYSINIT_H_

/*
 * Enumerated types for known system startup interfaces.
 *
 * Startup occurs in ascending numeric order; the list entries are
 * sorted prior to attempting startup to guarantee order.  Items
 * of the same level are arbitrated for order based on the 'order'
 * element.
 *
 * These numbers are arbitrary and are chosen ONLY for ordering; the
 * enumeration values are explicit rather than implicit to provide
 * for binary compatibility with inserted elements.
 *
 * The SI_SUB_LAST value must have the highest lexical value.
 */
enum sysinit_sub_id {
	SI_SUB_DUMMY				= 0x0000000,	/* not executed; for linker*/
	SI_SUB_DONE					= 0x0000001,	/* processed*/
	SI_SUB_TUNABLES				= 0x0700000,	/* establish tunable values */
	SI_SUB_COPYRIGHT			= 0x0800001,	/* first use of console*/
	SI_SUB_VM					= 0x1000000,	/* virtual memory system init */
	SI_SUB_COUNTER				= 0x1100000,	/* counter(9) is initialized */
	SI_SUB_KMEM					= 0x1800000,	/* kernel memory*/
	SI_SUB_HYPERVISOR			= 0x1A40000,	/*
						 	 	 	 	 	 	 * Hypervisor detection and
						 	 	 	 	 	 	 * virtualization support
						 	 	 	 	 	 	 * setup.
						 	 	 	 	 	 	 */
	SI_SUB_WITNESS				= 0x1A80000,	/* witness initialization */
	SI_SUB_MTX_POOL_DYNAMIC		= 0x1AC0000,	/* dynamic mutex pool */
	SI_SUB_LOCK					= 0x1B00000,	/* various locks */
	SI_SUB_EVENTHANDLER			= 0x1C00000,	/* eventhandler init */
	SI_SUB_VNET_PRELINK			= 0x1E00000,	/* vnet init before modules */
	SI_SUB_KLD					= 0x2000000,	/* KLD and module setup */
	SI_SUB_CPU					= 0x2100000,	/* CPU resource(s)*/
	SI_SUB_RACCT				= 0x2110000,	/* resource accounting */
	SI_SUB_KDTRACE				= 0x2140000,	/* Kernel dtrace hooks */
	SI_SUB_RANDOM				= 0x2160000,	/* random number generator */
	SI_SUB_MAC					= 0x2180000,	/* TrustedBSD MAC subsystem */
	SI_SUB_MAC_POLICY			= 0x21C0000,	/* TrustedBSD MAC policies */
	SI_SUB_MAC_LATE				= 0x21D0000,	/* TrustedBSD MAC subsystem */
	SI_SUB_VNET					= 0x21E0000,	/* vnet 0 */
	SI_SUB_INTRINSIC			= 0x2200000,	/* proc 0*/
	SI_SUB_VM_CONF				= 0x2300000,	/* config VM, set limits*/
	SI_SUB_DDB_SERVICES			= 0x2380000,	/* capture, scripting, etc. */
	SI_SUB_RUN_QUEUE			= 0x2400000,	/* set up run queue*/
	SI_SUB_KTRACE				= 0x2480000,	/* ktrace */
	SI_SUB_OPENSOLARIS			= 0x2490000,	/* OpenSolaris compatibility */
	SI_SUB_AUDIT				= 0x24C0000,	/* audit */
	SI_SUB_CREATE_INIT			= 0x2500000,	/* create init process*/
	SI_SUB_SCHED_IDLE			= 0x2600000,	/* required idle procs */
	SI_SUB_MBUF					= 0x2700000,	/* mbuf subsystem */
	SI_SUB_INTR					= 0x2800000,	/* interrupt threads */
	SI_SUB_TASKQ				= 0x2880000,	/* task queues */
	SI_SUB_EPOCH				= 0x2888000,	/* epoch subsystem */
#ifdef EARLY_AP_STARTUP
	SI_SUB_SMP					= 0x2900000,	/* start the APs*/
#endif
	SI_SUB_SOFTINTR				= 0x2A00000,	/* start soft interrupt thread */
	SI_SUB_DEVFS				= 0x2F00000,	/* devfs ready for devices */
	SI_SUB_INIT_IF				= 0x3000000,	/* prep for net interfaces */
	SI_SUB_NETGRAPH				= 0x3010000,	/* Let Netgraph initialize */
	SI_SUB_DTRACE				= 0x3020000,	/* DTrace subsystem */
	SI_SUB_DTRACE_PROVIDER		= 0x3048000,	/* DTrace providers */
	SI_SUB_DTRACE_ANON			= 0x308C000,	/* DTrace anon enabling */
	SI_SUB_DRIVERS				= 0x3100000,	/* Let Drivers initialize */
	SI_SUB_CONFIGURE			= 0x3800000,	/* Configure devices */
	SI_SUB_VFS					= 0x4000000,	/* virtual filesystem*/
	SI_SUB_CLOCKS				= 0x4800000,	/* real time and stat clocks*/
	SI_SUB_SYSV_SHM				= 0x6400000,	/* System V shared memory*/
	SI_SUB_SYSV_SEM				= 0x6800000,	/* System V semaphores*/
	SI_SUB_SYSV_MSG				= 0x6C00000,	/* System V message queues*/
	SI_SUB_P1003_1B				= 0x6E00000,	/* P1003.1B realtime */
	SI_SUB_PSEUDO				= 0x7000000,	/* pseudo devices*/
	SI_SUB_EXEC					= 0x7400000,	/* execve() handlers */
	SI_SUB_PROTO_BEGIN			= 0x8000000,	/* VNET initialization */
	SI_SUB_PROTO_PFIL			= 0x8100000,	/* Initialize pfil before FWs */
	SI_SUB_PROTO_IF				= 0x8400000,	/* interfaces*/
	SI_SUB_PROTO_DOMAININIT		= 0x8600000,	/* domain registration system */
	SI_SUB_PROTO_MC				= 0x8700000,	/* Multicast */
	SI_SUB_PROTO_DOMAIN			= 0x8800000,	/* domains (address families?)*/
	SI_SUB_PROTO_FIREWALL		= 0x8806000,	/* Firewalls */
	SI_SUB_PROTO_IFATTACHDOMAIN = 0x8808000,	/* domain dependent data init */
	SI_SUB_PROTO_END			= 0x8ffffff,	/* VNET helper functions */
	SI_SUB_KPROF				= 0x9000000,	/* kernel profiling*/
	SI_SUB_KICK_SCHEDULER		= 0xa000000,	/* start the timeout events*/
	SI_SUB_INT_CONFIG_HOOKS		= 0xa800000,	/* Interrupts enabled config */
	SI_SUB_ROOT_CONF			= 0xb000000,	/* Find root devices */
	SI_SUB_INTRINSIC_POST		= 0xd000000,	/* proc 0 cleanup*/
	SI_SUB_SYSCALLS				= 0xd800000,	/* register system calls */
	SI_SUB_VNET_DONE			= 0xdc00000,	/* vnet registration complete */
	SI_SUB_KTHREAD_INIT			= 0xe000000,	/* init process*/
	SI_SUB_KTHREAD_PAGE			= 0xe400000,	/* pageout daemon*/
	SI_SUB_KTHREAD_VM			= 0xe800000,	/* vm daemon*/
	SI_SUB_KTHREAD_BUF			= 0xea00000,	/* buffer daemon*/
	SI_SUB_KTHREAD_UPDATE		= 0xec00000,	/* update daemon*/
	SI_SUB_KTHREAD_IDLE			= 0xee00000,	/* idle procs*/
#ifndef EARLY_AP_STARTUP
	SI_SUB_SMP					= 0xf000000,	/* start the APs*/
#endif
	SI_SUB_RACCTD				= 0xf100000,	/* start racctd*/
	SI_SUB_LAST					= 0xfffffff		/* final initialization */
};

/*
 * Some enumerated orders; "ANY" sorts last.
 */
enum sysinit_elem_order {
	SI_ORDER_FIRST				= 0x0000000,	/* first*/
	SI_ORDER_SECOND				= 0x0000001,	/* second*/
	SI_ORDER_THIRD				= 0x0000002,	/* third*/
	SI_ORDER_FOURTH				= 0x0000003,	/* fourth*/
	SI_ORDER_FIFTH				= 0x0000004,	/* fifth*/
	SI_ORDER_SIXTH				= 0x0000005,	/* sixth*/
	SI_ORDER_SEVENTH			= 0x0000006,	/* seventh*/
	SI_ORDER_EIGHTH				= 0x0000007,	/* eighth*/
	SI_ORDER_MIDDLE				= 0x1000000,	/* somewhere in the middle */
	SI_ORDER_ANY				= 0xfffffff		/* last*/
};


/*
 * A system initialization call instance
 *
 * At the moment there is one instance of sysinit.  We probably do not
 * want two which is why this code is if'd out, but we definitely want
 * to discern SYSINIT's which take non-constant data pointers and
 * SYSINIT's which take constant data pointers,
 *
 * The C_* macros take functions expecting const void * arguments
 * while the non-C_* macros take functions expecting just void * arguments.
 *
 * With -Wcast-qual on, the compiler issues warnings:
 *	- if we pass non-const data or functions taking non-const data
 *	  to a C_* macro.
 *
 *	- if we pass const data to the normal macros
 *
 * However, no warning is issued if we pass a function taking const data
 * through a normal non-const macro.  This is ok because the function is
 * saying it won't modify the data so we don't care whether the data is
 * modifiable or not.
 */

typedef void (*sysinit_nfunc_t)(void *);
typedef void (*sysinit_cfunc_t)(const void *);

struct sysinit {
	enum sysinit_sub_id		subsystem;		/* subsystem identifier*/
	enum sysinit_elem_order	order;			/* init order within subsystem*/
	sysinit_cfunc_t 		func;			/* function		*/
	const void				*udata;			/* multiplexer/argument */
};

#endif /* SYS_DEVEL_SYSINIT_H_ */
