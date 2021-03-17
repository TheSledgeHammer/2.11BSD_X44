/*
 * Kernel interface to machine-dependent clock driver.
 * Garrett Wollman, September 1994.
 * This file is in the public domain.
 *
 * $FreeBSD$
 */

#ifndef _MACHINE_CLOCK_H_
#define	_MACHINE_CLOCK_H_

//#ifdef _KERNEL
/*
 * i386 to clock driver interface.
 * XXX large parts of the driver and its interface are misplaced.
 */
//extern int		adjkerntz;
//extern int		disable_rtc_set;
//extern int		statclock_disable;
//extern u_int	timer_freq;
//extern int		timer0_max_count;
extern u_int	tsc_freq;
//extern int		tsc_is_broken;
//extern int		wall_cmos_clock;
#ifdef APIC_IO
extern int		apic_8254_intr;
#endif

/*
 * Driver to clock driver interface.
 */

void	startrtclock(void);
int		clockintr(void *);
int		gettick();
void	delay(int);
void	sysbeepstop(void);
int		sysbeep(int, int);
void	findcpuspeed(void);
void	cpu_initclocks(void);
void	rtcinit(void);
int		rtcget(mc_todregs *);
void	rtcput(mc_todregs *);
int 	yeartoday (int);
int		bcdtobin(int);
int		bintobcd(int);
void 	inittodr(time_t);
void	resettodr(void);
void	setstatclockrate(int);
#endif /* _KERNEL */

#endif /* !_MACHINE_CLOCK_H_ */
