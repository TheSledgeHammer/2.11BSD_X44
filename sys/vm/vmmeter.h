/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vmmeter.h	7.1 (Berkeley) 6/4/86
 */
#ifndef _SYS_VMMETER_H_
#define _SYS_VMMETER_H_
/*
 * Virtual memory related instrumentation
 */
struct vmrate
{
	/* General system activity. */

	unsigned 	v_swtch;		/* context switches */
	unsigned 	v_trap;			/* calls to trap */
	unsigned 	v_syscall;		/* calls to syscall() */
	unsigned 	v_intr;			/* device interrupts */
	unsigned 	v_soft;			/* software interrupts */
	unsigned 	v_faults;		/* total faults taken */

	/* Virtual memory activity. */

	unsigned 	v_pdma;			/* pseudo-dma interrupts */
	unsigned 	v_ovly;			/* overlay emts */
	unsigned 	v_fpsim;		/* floating point simulator faults */
	unsigned 	v_pswpin;		/* pages swapped in (swap pager pages paged in) */
	unsigned 	v_pswpout;		/* pages swapped out (swap pager pages paged out) */
	unsigned 	v_pgin;			/* pageins */
	unsigned 	v_pgout;		/* pageouts */
	unsigned 	v_swpin;		/* swapins (swap pager pageins) */
	unsigned 	v_swpout;		/* swapouts (swap pager pageouts) */

	unsigned 	v_lookups;		/* object cache lookups */
	unsigned 	v_hits;			/* object cache hits */
	unsigned 	v_vm_faults;	/* number of address memory faults */
	unsigned 	v_cow_faults;	/* number of copy-on-writes */
	unsigned 	v_vnodein;		/* vnode pager pageins */
	unsigned 	v_vnodeout;		/* vnode pager pageouts */
	unsigned	v_vnodepgsin;	/* vnode_pager pages paged in */
	unsigned 	v_vnodepgsout;	/* vnode pager pages paged out */
	unsigned 	v_intrans;		/* intransit blocking page faults */
	unsigned 	v_reactivated;	/* number of pages reactivated from free list */
	unsigned 	v_pdwakeups;	/* number of times daemon has awaken from sleep */
	unsigned 	v_pdpages;		/* number of pages analyzed by daemon */
	unsigned 	v_dfree;		/* pages freed by daemon */
	unsigned 	v_pfree;		/* pages freed by exiting processes */
	unsigned 	v_tfree;		/* total pages freed */
	unsigned 	v_zfod;			/* pages zero filled on demand */
	unsigned 	v_nzfod;		/* number of zfod's created */

	/* Distribution of page usages. */

	unsigned 	v_page_size;		/* page size in bytes */
	unsigned 	v_page_count;		/* total number of pages in system */
	unsigned 	v_free_reserved; 	/* number of pages reserved for deadlock */
	unsigned 	v_free_target;		/* number of pages desired free */
	unsigned 	v_free_min;			/* minimum number of pages desired free */
	unsigned 	v_free_count;		/* number of pages free */
	unsigned 	v_wire_count;		/* number of pages wired down */
	unsigned 	v_active_count;		/* number of pages active */
	unsigned 	v_inactive_target; 	/* number of pages desired inactive */
	unsigned 	v_inactive_count;  	/* number of pages inactive */
};
#define	v_first	v_swtch
#define	v_last	v_pswpout

struct vmsum
{
	long	v_swtch;	/* context switches */
	long	v_trap;		/* calls to trap */
	long	v_syscall;	/* calls to syscall() */
	long	v_intr;		/* device interrupts */
	long	v_soft;		/* software interrupts */
	long	v_pdma;		/* pseudo-dma interrupts */
	long	v_ovly;		/* overlay emts */
	long	v_fpsim;	/* floating point simulator faults */
	long	v_pswpin;	/* pages swapped in */
	long	v_pswpout;	/* pages swapped out */
	long	v_pgin;		/* pageins */
	long	v_pgout;	/* pageouts */
	long	v_swpin;	/* swapins */
	long	v_swpout;	/* swapouts */
};
//#ifdef KERNEL
struct vmrate	cnt, rate;
struct vmsum	sum;
#endif

/* systemwide totals computed every five seconds */
struct vmtotal
{
	short	t_rq;		/* length of the run queue */
	short	t_dw;		/* jobs in ``disk wait'' (neg priority) */
	short	t_pw;		/* jobs in page wait */
	short	t_sl;		/* jobs sleeping in core */
	short	t_sw;		/* swapped out runnable/short block jobs */
	long	t_vm;		/* total virtual memory, clicks */
	long	t_avm;		/* active virtual memory, clicks */
	long	t_rm;		/* total real memory, clicks */
	long	t_arm;		/* active real memory, clicks */
	long	t_vmshr;	/* virtual memory used by text, clicks */
	long	t_avmshr;	/* active virtual memory used by text, clicks */
	long	t_rmshr;	/* real memory used by text, clicks */
	long	t_armshr;	/* active real memory used by text, clicks */
	long	t_free;		/* free memory pages, kb */

	long	t_vmtxt;	/* virtual memory used by text, clicks */
	long	t_avmtxt;	/* active virtual memory used by text, clicks */
	long	t_rmtxt;	/* real memory used by text, clicks */
	long	t_armtxt;	/* active real memory used by text, clicks */
};
//#ifdef KERNEL
struct	vmtotal total;
#endif

/* Optional instrumentation. */
#ifdef PGINPROF

#define	NDMON	128
#define	NSMON	128

#define	DRES	20
#define	SRES	5

#define	PMONMIN	20
#define	PRES	50
#define	NPMON	64

#define	RMONMIN	130
#define	RRES	5
#define	NRMON	64

/* data and stack size distribution counters */
unsigned int	dmon[NDMON+1];
unsigned int	smon[NSMON+1];

/* page in time distribution counters */
unsigned int	pmon[NPMON+2];

/* reclaim time distribution counters */
unsigned int	rmon[NRMON+2];

int	pmonmin;
int	pres;
int	rmonmin;
int	rres;

unsigned rectime;		/* accumulator for reclaim times */
unsigned pgintime;		/* accumulator for page in times */
#endif

#endif /* _SYS_VMMETER_H_ */
