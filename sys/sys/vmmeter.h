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
struct vmrate {
	/* General system activity. */
#define	v_first	v_swtch
	unsigned 	v_swtch;					/* context switches */
	unsigned 	v_trap;						/* calls to trap */
	unsigned 	v_syscall;					/* calls to syscall() */
	unsigned 	v_intr;						/* device interrupts */
	unsigned 	v_soft;						/* software interrupts */
	unsigned 	v_faults;					/* total faults taken */

	/* Virtual memory activity. */
	unsigned 	v_pdma;						/* pseudo-dma interrupts */
	unsigned 	v_ovly;						/* overlay emts */
	unsigned 	v_fpsim;					/* floating point simulator faults */
	unsigned 	v_pswpin;					/* pages swapped in */
	unsigned 	v_pswpout;					/* pages swapped out */
	unsigned 	v_pgin;						/* pageins */
	unsigned 	v_pgout;					/* pageouts */
#define	v_last	v_pswpout
	unsigned 	v_swpin;					/* swapins */
	unsigned 	v_swpout;					/* swapouts */

	unsigned 	v_lookups;					/* object cache lookups */
	unsigned 	v_hits;						/* object cache hits */
	unsigned 	v_vm_faults;				/* number of address memory faults */
	unsigned 	v_cow_faults;				/* number of copy-on-writes */
	unsigned 	v_pageins;					/* number of pageins */
	unsigned 	v_pageouts;					/* number of pageouts */
	unsigned	v_pgpgin;					/* pages paged in */
	unsigned 	v_pgpgout;					/* pages paged out */
	unsigned 	v_intrans;					/* intransit blocking page faults */
	unsigned 	v_reactivated;				/* number of pages reactivated from free list */
	unsigned 	v_rev;						/* revolutions of the hand */
	unsigned 	v_scan;						/* scans in page out daemon */
	unsigned 	v_dfree;					/* pages freed by daemon */
	unsigned 	v_pfree;					/* pages freed by exiting processes */
	unsigned 	v_tfree;					/* total pages freed */
	unsigned 	v_zfod;						/* pages zero filled on demand */
	unsigned 	v_nzfod;					/* number of zfod's created */

	/* Distribution of swap usages */
	unsigned	v_nswapdev;					/* number of configured swap devices in system */
	unsigned	v_swpages;					/* number of PAGE_SIZE'ed swap pages */
	unsigned 	v_swpgavail;				/* number of swap pages currently available */
	unsigned 	v_swpginuse;				/* number of swap pages in use */
	unsigned 	v_swpgonly;					/* number of swap pages in use, not also in RAM */
	unsigned 	v_nswget;					/* number of times fault calls uvm_swap_get() */

	/* Distribution of anon usages. */
	unsigned	v_nanon;					/* total number of anons in system */
	unsigned	v_nfreeanon;				/* number of anons free */
	unsigned    v_flt_acow;					/* number of times fault anon cow (case 1b) */
	unsigned    v_fltnoanon;				/* number of times fault was out of anons */
	unsigned    v_fltnoram;					/* number of times fault was out of ram */
	unsigned    v_flt_anon;					/* number of times fault anon (case 1a) */
	unsigned    v_fltanget;					/* number of times fault gets anon page */
	unsigned    v_fltanretry;				/* number of times fault retrys an anon get */
	unsigned    v_fltrelck;					/* number of times fault relock called */
	unsigned    v_fltrelckok;				/* number of times fault relock is a success */

	/* Distribution of page usages. */
	unsigned 	v_page_size;				/* page size in bytes */
	unsigned 	v_page_in_kernel;			/* total number of pages in system */
	unsigned 	v_page_free_target;			/* number of pages desired free */
	unsigned 	v_page_free_min;			/* minimum number of pages desired free */
	unsigned 	v_page_free_count;			/* number of pages free */
	unsigned 	v_page_wire_count;			/* number of pages wired down */
	unsigned 	v_page_active_count;		/* number of pages active */
	unsigned 	v_page_inactive_target; 	/* number of pages desired inactive */
	unsigned 	v_page_inactive_count;  	/* number of pages inactive */

	/* Distribution of segment usages */
	unsigned 	v_segment_size;				/* segment size in bytes */
	unsigned 	v_segment_in_kernel;		/* total number of segments in system */
	unsigned 	v_segment_free_count;		/* number of segments free */
	unsigned 	v_segment_active_count;		/* number of segments active */
	unsigned 	v_segment_inactive_count;  	/* number of segments inactive */
};

struct vmsum {
	long		v_swtch;					/* context switches */
	long		v_trap;						/* calls to trap */
	long		v_syscall;					/* calls to syscall() */
	long		v_intr;						/* device interrupts */
	long		v_soft;						/* software interrupts */
	long		v_faults;					/* total faults taken */

	long		v_pdma;						/* pseudo-dma interrupts */
	long		v_ovly;						/* overlay emts */
	long		v_fpsim;					/* floating point simulator faults */
	long		v_pswpin;					/* pages swapped in */
	long		v_pswpout;					/* pages swapped out */
	long		v_pgin;						/* pageins */
	long		v_pgout;					/* pageouts */
	long		v_swpin;					/* swapins */
	long		v_swpout;					/* swapouts */

	long 		v_lookups;					/* object cache lookups */
	long 		v_hits;						/* object cache hits */
	long 		v_vm_faults;				/* number of address memory faults */
	long 		v_cow_faults;				/* number of copy-on-writes */
	long 		v_pageins;					/* number of pageins */
	long 		v_pageouts;					/* number of pageouts */
	long		v_pgpgin;					/* pages paged in */
	long 		v_pgpgout;					/* pages paged out */
	long 		v_intrans;					/* intransit blocking page faults */
	long 		v_reactivated;				/* number of pages reactivated from free list */
	long 		v_rev;						/* revolutions of the hand */
	long 		v_scan;						/* scans in page out daemon */
	long 		v_dfree;					/* pages freed by daemon */
	long 		v_pfree;					/* pages freed by exiting processes */
	long 		v_tfree;					/* total pages freed */
	long 		v_zfod;						/* pages zero filled on demand */
	long 		v_nzfod;					/* number of zfod's created */

	/* Distribution of swap usages */
	long		v_nswapdev;					/* number of configured swap devices in system */
	long		v_swpages;					/* number of PAGE_SIZE'ed swap pages */
	long 		v_swpgavail;				/* number of swap pages currently available */
	long 		v_swpginuse;				/* number of swap pages in use */
	long 		v_swpgonly;					/* number of swap pages in use, not also in RAM */
	long 		v_nswget;					/* number of times fault calls uvm_swap_get() */

	/* Distribution of anon usages. */
	long		v_nanon;					/* total number of anons in system */
	long		v_nfreeanon;				/* number of anons free */
	long        v_flt_acow;					/* number of times fault anon cow (case 1b) */
	long        v_fltnoanon;				/* number of times fault was out of anons */
	long        v_fltnoram;					/* number of times fault was out of ram */
	long        v_flt_anon;					/* number of times fault anon (case 1a) */
	long        v_fltanget;					/* number of times fault gets anon page */
	long        v_fltanretry;				/* number of times fault retrys an anon get */
	long        v_fltrelck;					/* number of times fault relock called */
	long        v_fltrelckok;				/* number of times fault relock is a success */

	/* Distribution of page usages. */
	long 		v_page_size;				/* page size in bytes */
	long 		v_page_in_kernel;			/* total number of pages in system */
	long 		v_page_free_target;			/* number of pages desired free */
	long 		v_page_free_min;			/* minimum number of pages desired free */
	long 		v_page_free_count;			/* number of pages free */
	long 		v_page_wire_count;			/* number of pages wired down */
	long 		v_page_active_count;		/* number of pages active */
	long 		v_page_inactive_target; 	/* number of pages desired inactive */
	long 		v_page_inactive_count;  	/* number of pages inactive */

	/* Distribution of segment usages */
	long 		v_segment_size;				/* segment size in bytes */
	long 		v_segment_in_kernel;		/* total number of segments in system */
	long 		v_segment_free_count;		/* number of segments free */
	long 		v_segment_active_count;		/* number of segments active */
	long 		v_segment_inactive_count;  	/* number of segments inactive */
};

/* systemwide totals computed every five seconds */
struct vmtotal {
	short		t_rq;						/* length of the run queue */
	short		t_dw;						/* jobs in ``disk wait'' (neg priority) */
	short		t_pw;						/* jobs in page wait */
	short		t_sl;						/* jobs sleeping in core */
	short		t_sw;						/* swapped out runnable/short block jobs */
	long		t_vm;						/* total virtual memory, clicks */
	long		t_avm;						/* active virtual memory, clicks */
	long		t_rm;						/* total real memory, clicks */
	long		t_arm;						/* active real memory, clicks */
	long		t_vmtxt;					/* virtual memory used by text, clicks */
	long		t_avmtxt;					/* active virtual memory used by text, clicks */
	long		t_rmtxt;					/* real memory used by text, clicks */
	long		t_armtxt;					/* active real memory used by text, clicks */
	long		t_free;						/* free memory pages, kb */
};

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

#ifdef _KERNEL
extern struct vmrate	cnt, rate;
extern struct vmsum		sum;
extern struct vmtotal 	total;
#endif
#endif /* _SYS_VMMETER_H_ */
