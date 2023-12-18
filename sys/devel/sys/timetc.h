/* $NetBSD: timetc.h,v 1.9 2020/09/04 00:36:07 thorpej Exp $ */

/*-
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * $FreeBSD: src/sys/sys/timetc.h,v 1.58 2003/08/16 08:23:52 phk Exp $
 */

#ifndef _SYS_TIMETC_H_
#define _SYS_TIMETC_H_

#if !defined(_KERNEL) && !defined(_KMEMUSER)
#error "no user-serviceable parts inside"
#endif

/* place in sysctl.h */
/*
 * KERN_TIMECOUNTER
 */
#define KERN_TIMECOUNTER_TICK				1	/* int: number of revolutions */
#define KERN_TIMECOUNTER_TIMESTEPWARNINGS 	2	/* int: log a warning when time changes */
#define KERN_TIMECOUNTER_HARDWARE			3	/* string: tick hardware used */
#define KERN_TIMECOUNTER_CHOICE				4	/* string: tick hardware used */
#define KERN_TIMECOUNTER_MAXID				5

#define CTL_KERN_TIMECOUNTER_NAMES { 		\
	{ 0, 0 }, 								\
	{ "tick", CTLTYPE_INT }, 				\
	{ "timestepwarnings", CTLTYPE_INT }, 	\
	{ "hardware", CTLTYPE_STRING }, 		\
	{ "choice", CTLTYPE_STRING }, 			\
}

struct timecounter;
struct timespec;

typedef u_int timecounter_get_t(struct timecounter *);
typedef void timecounter_pps_t(struct timecounter *);

struct timecounter {
	timecounter_get_t	*tc_get_timecount;
			/*
			 * This function reads the counter.  It is not required to
			 * mask any unimplemented bits out, as long as they are
			 * constant.
			 */
	timecounter_pps_t	*tc_poll_pps;
			/*
			 * This function is optional.  It will be called whenever the
			 * timecounter is rewound, and is intended to check for PPS
			 * events.  Normal hardware does not need it but timecounters
			 * which latch PPS in hardware (like sys/pci/xrpu.c) do.
			 */
	u_int 				tc_counter_mask;
			/* This mask should mask off any unimplemented bits. */
	uint64_t			tc_frequency;
			/* Frequency of the counter in Hz. */
	const char			*tc_name;
			/* Name of the timecounter. */
	int					tc_quality;
			/*
			 * Used to determine if this timecounter is better than
			 * another timecounter higher means better.  Negative
			 * means "only use at explicit request".
			 */

	void				*tc_priv;		/* Pointer to the timecounter's private parts. */
	struct timecounter	*tc_next;		/* Pointer to the next timecounter. */
};

#ifdef _KERNEL
extern struct timecounter *timecounter;

u_int64_t tc_getfrequency(void);
void	tc_init(struct timecounter *tc);
int		tc_detach(struct timecounter *);
void	tc_setclock(struct timespec *ts);
void	tc_ticktock(void);
void	tc_gonebad(struct timecounter *);

#ifdef SYSCTL_DECL
int		sysctl_timecounter(int *, u_int, void *, size_t *, void *, size_t);
#endif
#endif /* _KERNEL */

#endif /* _SYS_TIMETC_H_ */
