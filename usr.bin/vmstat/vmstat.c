/*
/*
 * Copyright (c) 1980, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>

#if	defined(DOSCCS) && !defined(lint)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)vmstat.c	5.4.2 (2.11BSD GTE) 1997/3/28";
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/namei.h>
#include <sys/malloc.h>
#include <sys/signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/disk.h>
#include <sys/vmmeter.h>
#include <vm/include/vm.h>

#include <time.h>
#include <nlist.h>
#include <kvm.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <paths.h>
#include <limits.h>

#define NEWVM			/* XXX till old has been updated or purged */
struct nlist namelist[] = {
#define	X_CPTIME		0
		{ .n_name = "_cp_time" },
#define	X_RATE			1
		{ .n_name = "_rate" },
#define X_TOTAL			2
		{ .n_name = "_total" },
#define	X_DEFICIT		3
		{ .n_name = "_deficit" },
#define	X_FORKSTAT		4
		{ .n_name = "_forkstat" },
#define X_SUM			5
		{ .n_name = "_sum" },
#define	X_FIRSTFREE		6
		{ .n_name = "_firstfree" },
#define	X_MAXFREE		7
		{ .n_name = "_maxfree" },
#define	X_BOOTTIME		8
		{ .n_name = "_boottime" },
#define	X_DKXFER		9
		{ .n_name = "_dk_xfer" },
#define X_REC			10
		{ .n_name = "_rectime" },
#define X_PGIN			11
		{ .n_name = "_pgintime" },
#define X_HZ			12
		{ .n_name = "_hz" },
#define X_PHZ			13
		{ .n_name = "_phz" },
#define X_STATHZ		14
		{ .n_name = "_stathz" },
#define X_NCHSTATS		15
		{ .n_name = "_nchstats" },
#define	X_INTRNAMES		16
		{ .n_name = "_intrnames" },
#define	X_EINTRNAMES	17
		{ .n_name = "_eintrnames" },
#define	X_INTRCNT		18
		{ .n_name = "_intrcnt" },
#define	X_EINTRCNT		19
		{ .n_name = "_eintrcnt" },
#define	X_KMEMSTAT		20
		{ .n_name = "_kmemstats" },
#define	X_KMEMBUCKETS	21
		{ .n_name = "_bucket" },
#define	X_DK_NDRIVE		22
		{ .n_name = "_dk_ndrive" },
#define	X_XSTATS		23
		{ .n_name = "_xstats" },
#define	X_DK_NAME		24
		{ .n_name = "_dk_name" },
#define	X_DK_UNIT		25
		{ .n_name = "_dk_unit" },
#define	X_DK_DISKLIST 	26
		{ .n_name = "_dk_disklist" },
#define X_FREEMEM		27
		{ .n_name = "_freemem" },
#define X_END			28
		{ .n_name = NULL },
};

struct disk_sysctl {
	int					busy;
	long				time[CPUSTATES];
	long				*xfer;
} cur, last;

static struct dkdevice *dk_drivehead = NULL;
char **dk_name;
int	 *dk_unit;
char **dr_name;
int	 *dr_select;
int	 dk_ndrive;
int	 ndrives = 0;

struct vm_sysctl {
	struct	vmrate   	Rate;
	struct	vmtotal	 	Total;
	struct	vmsum    	Sum;
	struct	forkstat 	Forkstat;
	unsigned 			rectime;
	unsigned 			pgintime;
} vms, vms1;

#define rate 		vms.Rate
#define total 		vms.Total
#define	sum			vms.Sum
#define	forkstat	vms.Forkstat

struct vmsum osum;
struct vm_xstats pxstats, cxstats;
static kvm_t *kd;

char *defdrives[] = { 0 };

int	hz, phz, HZ, hdrcnt;
int deficit;
int	winlines = 20;
size_t	pfree;
int	firstfree, maxfree;

#define	FORKSTAT	0x01
#define	INTRSTAT	0x02
#define	MEMSTAT		0x04
#define	SUMSTAT		0x08
#define	TIMESTAT	0x10
#define	VMSTAT		0x20

static char **getdrivedata(char **);
static void dorate(time_t);
static long getuptime(void);
static void	dovmstat(unsigned int, int);
static void printhdr(void);
static void needhdr(void);
static long	pct(long, long);
static void dotimes(void);
static void	dosum(void);
static void	doforkst(void);
static void	dkstats(void);
static void	cpustats(void);
static void	dointr(void);
static void	domem(void);
static int dkinit(char *, char *, int);
static void	kread(int, void *, size_t);
static void kread2(void *, void *, size_t);
static void	usage(void);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	extern char *optarg;
	register int c, todo;
	u_int interval;
	int reps;
	char *memf, *nlistf;
    char errbuf[_POSIX2_LINE_MAX];

	memf = nlistf = NULL;
	interval = reps = todo = 0;
	while ((c = getopt(argc, argv, "ac:fhHiM:mN:n:oPp:sw:z")) != -1) {
		switch (c) {
		case 'c':
			reps = atoi(optarg);
			break;
		case 'f':
			todo |= FORKSTAT;
			break;
		case 'i':
			todo |= INTRSTAT;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			todo |= MEMSTAT;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 's':
			todo |= SUMSTAT;
			break;
		case 't':
			todo |= TIMESTAT;
			break;
		case 'w':
			interval = atoi(optarg);
			break;
		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (todo == 0) {
		todo = VMSTAT;
	}

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL) {
		setgid(getgid());
	}

	kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf);
	if (kd == 0) {
		(void) fprintf(stderr, "vmstat: kvm_openfiles: %s\n", errbuf);
		exit(1);
	}
	if ((c = kvm_nlist(kd, namelist)) != 0) {
		if (c > 0) {
			(void) fprintf(stderr, "vmstat: undefined symbols:");
			for (c = 0; c < sizeof(namelist) / sizeof(namelist[0]); c++) {
				if (namelist[c].n_type == 0) {
					fprintf(stderr, " %s", namelist[c].n_name);
				}
			}
			(void) fputc('\n', stderr);
		} else {
			(void) fprintf(stderr, "vmstat: kvm_nlist: %s\n", kvm_geterr(kd));
		}
		exit(1);
	}

	if (todo & VMSTAT) {
		struct winsize winsize;

		dkinit(memf, nlistf, 0);	/* Initialize disk stats, no disks selected. */

		(void)setgid(getgid()); 	/* don't need privs anymore */

		argv = getdrivedata(argv);
		winsize.ws_row = 0;
		(void) ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&winsize);
		if (winsize.ws_row > 0) {
			winlines = winsize.ws_row;
		}
	}

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		interval = atoi(*argv);
		if (*++argv) {
			reps = atoi(*argv);
		}
	}
#endif

	if (interval) {
		if (!reps) {
			reps = -1;
		}
	} else if (reps) {
		interval = 1;
	}

	if (todo & FORKSTAT) {
		doforkst();
	}
	if (todo & MEMSTAT) {
		domem();
	}
	if (todo & SUMSTAT) {
		dosum();
	}
	if (todo & TIMESTAT) {
		dotimes();
	}
	if (todo & INTRSTAT) {
		dointr();
	}
	if (todo & VMSTAT) {
		dovmstat(interval, reps);
	}
	exit(0);
}

static char **
getdrivedata(argv)
	char **argv;
{
	register int i;
	register char **cp;
	char buf[30];

	/*
	 * Choose drives to be displayed.  Priority goes to (in order) drives
	 * supplied as arguments, default drives.  If everything isn't filled
	 * in and there are drives not taken care of, display the first few
	 * that fit.
	 */
#define BACKWARD_COMPATIBILITY
	for (ndrives = 0; *argv; ++argv) {
#ifdef	BACKWARD_COMPATIBILITY
		if (isdigit(**argv)) {
			break;
		}
#endif
		for (i = 0; i < dk_ndrive; i++) {
			if (strcmp(dr_name[i], *argv)) {
				continue;
			}
			dr_select[i] = 1;
			++ndrives;
			break;
		}
	}
	/*
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i])
			continue;
		for (cp = defdrives; *cp; cp++) {
			if (strcmp(dr_name[i], *cp) == 0) {
				dr_select[i] = 1;
				++ndrives;
				break;
			}
		}
	}
	*/
	for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
		if (dr_select[i]) {
			continue;
		}
		dr_select[i] = 1;
		++ndrives;
	}
	return (argv);
}

static long
getuptime(void)
{
	static time_t now, boottime;
	time_t uptime;

	if (boottime == 0) {
		kread(X_BOOTTIME, &boottime, sizeof(boottime));
	}
	/*
	kread(X_HZ, &hz, sizeof(hz));
	if (hz != 0) {
		kread(X_PHZ, &phz, sizeof(phz));
	}
	HZ = phz ? phz : hz;
	*/
	(void) time(&now);
	uptime = now - boottime;
	if (uptime <= 0 || uptime > 60 * 60 * 24 * 365 * 10) {
		(void) fprintf(stderr,
				"vmstat: time makes no sense; namelist must be wrong.\n");
		exit(1);
	}
	dorate(uptime);
	return (uptime);
}

static void
dovmstat(interval, reps)
	u_int interval;
	int reps;
{
	time_t uptime, halfuptime;
	int mib[2], size;

	uptime = getuptime();
	halfuptime = uptime / 2;
	(void)signal(SIGCONT, needhdr);

	if (namelist[X_STATHZ].n_type != 0 && namelist[X_STATHZ].n_value != 0)
		kread(X_STATHZ, &hz, sizeof(hz));
	if (!hz)
		kread(X_HZ, &hz, sizeof(hz));

	for (hdrcnt = 1;;) {
		if (!--hdrcnt) {
			printhdr();
		}
		kread(X_CPTIME, cur.time, sizeof(cur.time));
		kread(X_DKXFER, cur.xfer, sizeof(*cur.xfer) * dk_ndrive);
		kread(X_SUM, &sum, sizeof(sum));
		size = sizeof(total);
		mib[0] = CTL_VM;
		mib[1] = VM_METER;
		if (sysctl(mib, 2, &total, &size, NULL, 0) < 0) {
			printf("Can't get kerninfo: %s\n", strerror(errno));
			bzero(&total, sizeof(total));
		}
		(void)printf("%2d%2d%2d", total.t_rq - 1, total.t_dw + total.t_pw, total.t_sw);
#define pgtok(a) ((a) * sum.v_page_size >> 10)
#define	rate(x)	(((x) + halfuptime) / uptime)	/* round */

		(void)printf("%6ld%6ld ", pgtok(total.t_avm), pgtok(total.t_free));
		(void)printf("%4lu ", rate(sum.v_faults - osum.v_faults));
		(void)printf("%3lu ", rate(sum.v_reactivated - osum.v_reactivated));
		(void)printf("%3lu ", rate(sum.v_pageins - osum.v_pageins));
		(void)printf("%3lu %3lu ", rate(sum.v_pageouts - osum.v_pageouts), 0);
		(void)printf("%3lu ", rate(sum.v_scan - osum.v_scan));
		dkstats();
		(void)printf("%4lu %4lu %3lu ", rate(sum.v_intr - osum.v_intr),
				rate(sum.v_syscall - osum.v_syscall),
				rate(sum.v_swtch - osum.v_swtch));
		cpustats();
		(void) printf("\n");
		(void) fflush(stdout);
		if (reps >= 0 && --reps <= 0)
			break;
		osum = sum;
		uptime = interval;
		/*
		 * We round upward to avoid losing low-frequency events
		 * (i.e., >= 1 per interval but < 1 per second).
		 */
		halfuptime = (uptime + 1) / 2;
		(void) sleep(interval);
	}
}

static void
printhdr(void)
{
	register int i;

	(void) printf(" procs   memory     page%*s", 20, "");
	if (ndrives > 1) {
		(void) printf("disks %*s  faults      cpu\n", ndrives * 3 - 6, "");
	} else {
		(void) printf("%*s  faults      cpu\n", ndrives * 3, "");
	}
	(void)printf(" r b w   avm   fre  flt  re  pi  po  fr  sr ");

	for (i = 0; i < dk_ndrive; i++) {
		if (dr_select[i]) {
			(void) printf("%c%c ", dr_name[i][0], dr_name[i][strlen(dr_name[i]) - 1]);
		}
	}
	(void) printf("  in   sy  cs us sy id\n");
	hdrcnt = winlines - 2;
}

static void
needhdr(void)
{
	hdrcnt = 1;
}

static void
dotimes(void)
{
	kread(X_REC, &vms.rectime, sizeof(vms.rectime));
	kread(X_PGIN, &vms.pgintime, sizeof(vms.pgintime));
	kread(X_SUM, &sum, sizeof(sum));
	(void) printf("%u reclaims, %u total time (usec)\n", sum.v_pgrec,
			vms.rectime);
	(void) printf("average: %u usec / reclaim\n", vms.rectime / sum.v_pgrec);
	(void) printf("\n");
	(void) printf("%u page ins, %u total time (msec)\n", sum.v_pgin,
			vms.pgintime / 10);
	(void) printf("average: %8.1f msec / page in\n",
			vms.pgintime / (sum.v_pgin * 10.0));
}

static long
pct(top, bot)
	long top, bot;
{
	long ans;

	if (bot == 0)
		return(0);
	ans = (quad_t)top * 100 / bot;
	return (ans);
}

#define	PCT(top, bot) pct((long)(top), (long)(bot))

static void
dosum(void)
{
	struct nchstats   nchstats;
	struct vm_xstats  xstats;
	long nchtotal;

	kread(X_SUM, &sum, sizeof(sum));
	(void)printf("%9u cpu context switches\n", sum.v_swtch);
	(void)printf("%9u device interrupts\n", sum.v_intr);
	(void)printf("%9u software interrupts\n", sum.v_soft);
	(void)printf("%9u pseudo-dma dz interrupts\n", sum.v_pdma);
	(void)printf("%9u traps\n", sum.v_trap);
	(void)printf("%9u overlay emts\n", sum.v_ovly);
	(void)printf("%9u system calls\n", sum.v_syscall);
	(void)printf("%9u total faults taken\n", sum.v_faults);
	(void)printf("%9u swap ins\n", sum.v_swpin);
	(void)printf("%9u swap outs\n", sum.v_swpout);
	(void)printf("%9u pages swapped in\n", sum.v_pswpin / CLSIZE);
	(void)printf("%9u pages swapped out\n", sum.v_pswpout / CLSIZE);
	(void)printf("%9u page ins\n", sum.v_pageins);
	(void)printf("%9u page outs\n", sum.v_pageouts);
	(void)printf("%9u pages paged in\n", sum.v_pgpgin);
	(void)printf("%9u pages paged out\n", sum.v_pgpgout);
	(void)printf("%9u pages reactivated\n", sum.v_reactivated);
	(void)printf("%9u intransit blocking page faults\n", sum.v_intrans);
	(void)printf("%9u zero fill pages created\n", sum.v_nzfod / CLSIZE);
	(void)printf("%9u zero fill page faults\n", sum.v_zfod / CLSIZE);
	(void)printf("%9u pages examined by the clock daemon\n", sum.v_scan);
	(void)printf("%9u revolutions of the clock hand\n", sum.v_rev);
	(void)printf("%9u VM object cache lookups\n", sum.v_lookups);
	(void)printf("%9u VM object hits\n", sum.v_hits);
	(void)printf("%9u total VM faults taken\n", sum.v_vm_faults);
	(void)printf("%9u copy-on-write faults\n", sum.v_cow_faults);
	(void)printf("%9u pages freed by daemon\n", sum.v_dfree);
	(void)printf("%9u pages freed by exiting processes\n", sum.v_pfree);
	(void)printf("%9u swap devices\n", sum.v_nswapdev);
	(void)printf("%9u swap pages\n", sum.v_swpages);
	(void)printf("%9u swap pages available\n", sum.v_swpgavail);
	(void)printf("%9u swap pages in use\n", sum.v_swpginuse);
	(void)printf("%9u swap allocations\n", sum.v_nswget);
	(void)printf("%9u total anons\n", sum.v_nanon);
	(void)printf("%9u anons free\n", sum.v_nfreeanon);
	(void)printf("%9u faults with no anons\n", sum.v_fltnoanon);
	(void)printf("%9u faults with no memory\n", sum.v_fltnoram);
	(void)printf("%9u anon faults\n", sum.v_flt_anon);
	(void)printf("%9u anon copy on write faults\n", sum.v_flt_acow);
	(void)printf("%9u anon page faults\n", sum.v_fltanget);
	(void)printf("%9u anon retry faults\n", sum.v_fltanretry);
	(void)printf("%9u faults relock\n", sum.v_fltrelck);
	(void)printf("%9u faults relock ok\n", sum.v_fltrelckok);
	(void)printf("%9u segments free\n", sum.v_segment_free_count);
	(void)printf("%9u segments active\n", sum.v_segment_active_count);
	(void)printf("%9u segments inactive\n", sum.v_segment_inactive_count);
	(void)printf("%9u bytes per segments\n", sum.v_segment_size);
	(void)printf("%9u pages free\n", sum.v_page_free_count);
	(void)printf("%9u pages wired down\n", sum.v_page_wire_count);
	(void)printf("%9u pages active\n", sum.v_page_active_count);
	(void)printf("%9u pages inactive\n", sum.v_page_inactive_count);
	(void)printf("%9u bytes per page\n", sum.v_page_size);
	(void)printf("%9u target inactive pages\n", sum.v_page_inactive_target);
	(void)printf("%9u target free pages\n", sum.v_page_free_target);
	(void)printf("%9u minimum free pages\n", sum.v_page_free_min);

	kread(X_NCHSTATS, &nchstats, sizeof(nchstats));
	nchtotal = nchstats.ncs_goodhits + nchstats.ncs_neghits
			+ nchstats.ncs_badhits + nchstats.ncs_falsehits + nchstats.ncs_miss
			+ nchstats.ncs_long;
	(void) printf("%9ld total name lookups\n", nchtotal);
	(void) printf(
			"%9s cache hits (%d%% pos + %d%% neg) system %d%% per-process\n",
			"", PCT(nchstats.ncs_goodhits, nchtotal),
			PCT(nchstats.ncs_neghits, nchtotal),
			PCT(nchstats.ncs_pass2, nchtotal));
	(void) printf("%9s deletions %d%%, falsehits %d%%, toolong %d%%\n", "",
			PCT(nchstats.ncs_badhits, nchtotal),
			PCT(nchstats.ncs_falsehits, nchtotal),
			PCT(nchstats.ncs_long, nchtotal));

	kread(X_XSTATS, &xstats, sizeof(xstats));
	(void) printf("%9lu total calls to xalloc (cache hits %d%%)\n",
			xstats.psxs_alloc,
			PCT(xstats.psxs_alloc_cachehit, xstats.psxs_alloc));
	(void) printf("%9s sticky %lu flushed %lu unused %lu\n", "",
			xstats.psxs_alloc_inuse, xstats.psxs_alloc_cacheflush,
			xstats.psxs_alloc_unused);
	(void) printf("%9lu total calls to xfree", xstats.psxs_free);
	(void) printf(" (sticky %lu cached %lu swapped %lu)\n",
			xstats.psxs_free_inuse, xstats.psxs_free_cache,
			xstats.psxs_free_cacheswap);
}

static void
doforkst(void)
{
	kread(X_FORKSTAT, &forkstat, sizeof(forkstat));
	(void)printf("%d forks, %d pages, average %.2f\n", forkstat.cntfork,
			forkstat.sizfork, (double) forkstat.sizfork / forkstat.cntfork);
	(void)printf("%d vforks, %d pages, average %.2f\n", forkstat.cntvfork,
			forkstat.sizvfork, (double) forkstat.sizvfork / forkstat.cntvfork);
}

static void
dkstats(void)
{
	register int dn, state;
	double etime;
	long tmp;

	for (dn = 0; dn < dk_ndrive; ++dn) {
		tmp = cur.xfer[dn];
		cur.xfer[dn] -= last.xfer[dn];
		last.xfer[dn] = tmp;
	}
	etime = 0;
	for (state = 0; state < CPUSTATES; ++state) {
		tmp = cur.time[state];
		cur.time[state] -= last.time[state];
		last.time[state] = tmp;
		etime += cur.time[state];
	}
	if (etime == 0) {
		etime = 1;
	}
	etime /= HZ;
	for (dn = 0; dn < dk_ndrive; ++dn) {
		if (!dr_select[dn]) {
			continue;
		}
		(void) printf("%2.0f ", cur.xfer[dn] / etime);
	}
}

static void
cpustats(void)
{
	register int state;
	double pct, tot;

	tot = 0;
	for (state = 0; state < CPUSTATES; ++state) {
		tot += cur.time[state];
	}
	if (total) {
		pct = 100 / tot;
	} else {
		pct = 0;
	}
	(void)printf("%2.0f ", (cur.time[CP_USER] + cur.time[CP_NICE]) * pct);
	(void)printf("%2.0f ", (cur.time[CP_SYS] + cur.time[CP_INTR]) * pct);
	(void)printf("%2.0f", cur.time[CP_IDLE] * pct);
}

static void
dointr(void)
{
	register long *intrcnt, inttotal, uptime;
	register int nintr, inamlen;
	register char *intrname;

	uptime = getuptime();
	nintr = namelist[X_EINTRCNT].n_value - namelist[X_INTRCNT].n_value;
	inamlen = namelist[X_EINTRNAMES].n_value - namelist[X_INTRNAMES].n_value;
	intrcnt = malloc((size_t)nintr);
	intrname = malloc((size_t)inamlen);
	if (intrcnt == NULL || intrname == NULL) {
		(void)fprintf(stderr, "vmstat: %s.\n", strerror(errno));
		exit(1);
	}
	kread(X_INTRCNT, intrcnt, (size_t)nintr);
	kread(X_INTRNAMES, intrname, (size_t)inamlen);
	(void)printf("interrupt      total      rate\n");
	inttotal = 0;
	nintr /= sizeof(long);
	while (--nintr >= 0) {
		if (*intrcnt) {
			(void)printf("%-12s %8ld %8ld\n", intrname, *intrcnt, *intrcnt / uptime);
		}
		intrname += strlen(intrname) + 1;
		inttotal += *intrcnt++;
	}
	(void)printf("Total        %8ld %8ld\n", inttotal, inttotal / uptime);
}

static void
dorate(nitv)
	time_t nitv;
{
	kread(X_XSTATS, &cxstats, sizeof(cxstats));
	if (nitv != 1) {
		kread(X_SUM, &sum, sizeof(sum));
		rate.v_swtch = sum.v_swtch;
		rate.v_trap = sum.v_trap;
		rate.v_syscall = sum.v_syscall;
		rate.v_intr = sum.v_intr;
		rate.v_pdma = sum.v_pdma;
		rate.v_ovly = sum.v_ovly;
		rate.v_pgin = sum.v_pgin;
		rate.v_pgout = sum.v_pgout;
		rate.v_swpin = sum.v_swpin;
		rate.v_swpout = sum.v_swpout;
		rate.v_lookups = sum.v_lookups;
		rate.v_hits = sum.v_hits;
		rate.v_vm_faults = sum.v_vm_faults;
		rate.v_cow_faults = sum.v_cow_faults;
		rate.v_pageins = sum.v_pageins;
		rate.v_pageouts = sum.v_pageouts;
		rate.v_pgpgin = sum.v_pgpgin;
		rate.v_pgpgout = sum.v_pgpgout;
		rate.v_intrans = sum.v_intrans;
		rate.v_reactivated = sum.v_reactivated;
		rate.v_rev = sum.v_rev;
		rate.v_scan = sum.v_scan;
		rate.v_dfree = sum.v_dfree;
		rate.v_pfree = sum.v_pfree;
		rate.v_tfree = sum.v_tfree;
		rate.v_zfod = sum.v_zfod;
		rate.v_nzfod = sum.v_nzfod;
		rate.v_nswapdev = sum.v_nswapdev;
		rate.v_swpages = sum.v_swpages;
		rate.v_swpgavail = sum.v_swpgavail;
		rate.v_swpginuse = sum.v_swpginuse;
		rate.v_swpgonly = sum.v_swpgonly;
		rate.v_nswget = sum.v_nswget;
		rate.v_nanon = sum.v_nanon;
		rate.v_nfreeanon = sum.v_nfreeanon;
		rate.v_flt_acow = sum.v_flt_acow;
		rate.v_fltnoanon = sum.v_fltnoanon;
		rate.v_fltnoram = sum.v_fltnoram;
		rate.v_flt_anon = sum.v_flt_anon;
		rate.v_fltanget = sum.v_fltanget;
		rate.v_fltanretry = sum.v_fltanretry;
		rate.v_fltrelck = sum.v_fltrelck;
		rate.v_fltrelckok = sum.v_fltrelckok;
		rate.v_page_size = sum.v_page_size;
		rate.v_page_free_target = sum.v_page_free_target;
		rate.v_page_free_min = sum.v_page_free_min;
		rate.v_page_free_count = sum.v_page_free_count;
		rate.v_page_wire_count = sum.v_page_wire_count;
		rate.v_page_active_count = sum.v_page_active_count;
		rate.v_page_inactive_target = sum.v_page_inactive_target;
		rate.v_page_inactive_count = sum.v_page_inactive_count;
		rate.v_segment_size = sum.v_segment_size;
		rate.v_segment_free_count = sum.v_segment_free_count;
		rate.v_segment_active_count = sum.v_segment_active_count;
		rate.v_segment_inactive_count = sum.v_segment_inactive_count;
	} else {
		kread(X_RATE, &rate, sizeof(rate));
		kread(X_SUM, &sum, sizeof(sum));
	}
	pxstats = cxstats;
	kread(X_XSTATS, &cxstats, sizeof(cxstats));
	kread(X_FREEMEM, &pfree, sizeof(pfree));
	kread(X_TOTAL, &total, sizeof(total));
	osum = sum;
	kread(X_SUM, &sum, sizeof(sum));
	kread(X_DEFICIT, &deficit, sizeof(deficit));
}

/*
 * These names are defined in <sys/malloc.h>.
 */
char *kmemnames[] = INITKMEMNAMES;

static void
domem(void)
{
	register struct kmemslabs *slab;
	register struct kmembuckets *kp;
	register struct kmemstats *ks;
	register int i, j;
	int len, size, first;
	long totuse = 0, totfree = 0, totreq = 0;
	char *name;
	struct kmemstats kmemstats[M_LAST];
	struct kmemslabs slabbuckets[MINBUCKET + 16];

	kread(X_FIRSTFREE, &firstfree, sizeof(firstfree));
	kread(X_MAXFREE, &maxfree, sizeof(maxfree));
	kread(X_KMEMBUCKETS, slabbuckets, sizeof(slabbuckets));
	(void)printf("Memory statistics by bucket size\n");
	(void)printf(
	    "    Size   In Use   Free   Requests  HighWater  Couldfree\n");
	for (i = MINBUCKET, slab = &slabbuckets[i]; i < MINBUCKET + 16; i++, slab++) {
		kp = slab->ksl_bucket;
		if (kp->kb_calls == 0) {
			continue;
		}
		size = 1 << i;
		(void)printf("%8d %8ld %6ld %10ld %7ld %10ld\n", size,
			kp->kb_total - kp->kb_totalfree,
			kp->kb_totalfree, kp->kb_calls,
			kp->kb_highwat, kp->kb_couldfree);
		totfree += size * kp->kb_totalfree;
	}

	kread(X_KMEMSTAT, kmemstats, sizeof(kmemstats));
	(void)printf("\nMemory usage type by bucket size\n");
	(void)printf("    Size  Type(s)\n");
	slab = &slabbuckets[MINBUCKET];
	for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1, slab++) {
		kp = slab->ksl_bucket;
		if (kp->kb_calls == 0) {
			continue;
		}
		first = 1;
		len = 8;
		for (i = 0, ks = &kmemstats[0]; i < M_LAST; i++, ks++) {
			if (ks->ks_calls == 0) {
				continue;
			}
			if ((ks->ks_size & j) == 0) {
				continue;
			}
			name = kmemnames[i] ? kmemnames[i] : "undefined";
			len += 2 + strlen(name);
			if (first) {
				printf("%8d  %s", j, name);
			} else {
				printf(",");
			}
			if (len >= 80) {
				printf("\n\t ");
				len = 10 + strlen(name);
			}
			if (!first) {
				printf(" %s", name);
			}
			first = 0;
		}
		printf("\n");
	}

	(void)printf("\nMemory statistics by type                        Type  Kern\n");
	(void)printf("      Type  InUse MemUse HighUse  Limit Requests Limit Limit Size(s)\n");
	for (i = 0, ks = &kmemstats[0]; i < M_LAST; i++, ks++) {
		if (ks->ks_calls == 0) {
			continue;
		}
		(void) printf("%11s%6ld%6ldK%7ldK%6ldK%9ld%5u%6u",
				kmemnames[i] ? kmemnames[i] : "undefined", ks->ks_inuse,
				(ks->ks_memuse + 1023) / 1024, (ks->ks_maxused + 1023) / 1024,
				(ks->ks_limit + 1023) / 1024, ks->ks_calls, ks->ks_limblocks,
				ks->ks_mapblocks);
		first = 1;
		for (j =  1 << MINBUCKET; j < 1 << (MINBUCKET + 16); j <<= 1) {
			if ((ks->ks_size & j) == 0) {
				continue;
			}
			if (first) {
				printf("  %d", j);
			} else {
				printf(",%d", j);
			}
			first = 0;
		}
		printf("\n");
		totuse += ks->ks_memuse;
		totreq += ks->ks_calls;
	}
	(void)printf("\nMemory Totals:  In Use    Free    Requests\n");
	(void)printf("              %7ldK %6ldK    %8ld\n",
	     (totuse + 1023) / 1024, (totfree + 1023) / 1024, totreq);
}

/*
 * Perform all of the initialization and memory allocation needed to
 * track disk statistics.
 */
static int
dkinit(memf, nlistf, select)
	char *memf, *nlistf;
	int select;
{
	struct disklist_head disk_head;
	struct dkdevice	cur_disk, *p;
	struct clockinfo clockinfo;
	size_t size;
	int	i, mib[2];
	char buf[30];
	static int	once = 0;

	if (once) {
		return (1);
	}

	if (memf == NULL) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_CLOCKRATE;
		size = sizeof(clockinfo);
		if (sysctl(mib, 2, &clockinfo, &size, NULL, 0) < 0) {
			(void)fprintf(stderr, "vmstat: Can't get sysctl kern.clockrate %d\n", strerror(errno));
		}

		kread(X_HZ, &hz, sizeof(hz));
		if (hz != 0 || hz != clockinfo.stathz) {
			hz = clockinfo.hz;
			kread(X_PHZ, &phz, sizeof(phz));
		}
		HZ = phz ? phz : hz;

		mib[0] = CTL_HW;
		mib[1] = HW_DISKNAMES;
		size = 0;
		if (sysctl(mib, 2, NULL, &size, NULL, 0) < 0) {
			(void)fprintf(stderr, "vmstat: Can't get sysctl hw.diskstats %d\n", strerror(errno));
		}
		if (size == 0) {
			(void)fprintf(stderr, "vmstat: No drives attached %s\n", strerror(errno));
		}
	}

	kread(X_DK_NDRIVE, &dk_ndrive, sizeof(dk_ndrive));
	if (dk_ndrive <= 0) {
		(void)fprintf(stderr, "vmstat: invalid disk count dk_ndrive %d\n", dk_ndrive);
		exit(1);
	} else if (dk_ndrive == 0){
		(void)fprintf(stderr, "vmstat: no drives attached dk_ndrive %d\n", dk_ndrive);
		exit(1);
	} else {
		/* Get a pointer to the first disk. */
		kread(X_DK_DISKLIST, &disk_head, sizeof(disk_head));
		dk_drivehead = TAILQ_FIRST(disk_head);
	}

	/* Get ticks per second. */
	kread(X_STATHZ, &hz, sizeof(hz));
	if (hz != 0) {
		kread(X_HZ, &hz, sizeof(hz));
	}

	/* Allocate space for the statistics. */
	dr_select = (int *)calloc(dk_ndrive, sizeof (int));
	dr_name = (char **)calloc(dk_ndrive, sizeof (char *));
	dk_name = (char **)calloc(dk_ndrive, sizeof (char *));
	dk_unit = (int *)calloc(dk_ndrive, sizeof (int));
	cur.xfer = calloc((size_t)dk_ndrive, sizeof(long));
	last.xfer = calloc((size_t)dk_ndrive, sizeof(long));

	p = dk_drivehead;
	for (i = 0; i < dk_ndrive; i++) {
		(void)sprintf(buf, "dk%d", i);
		//(void)sprintf(buf, "??%d", i);
		kread2(p, &cur_disk, sizeof(cur_disk));
		kread2(cur_disk.dk_name, buf, sizeof(buf));
		dr_name[i] = strdup(buf);
		if (dr_name[i] == NULL) {
			err(1, "strdup");
		}
		dr_select[i] = select;
		p = TAILQ_NEXT(cur_disk, dk_link);
	}

	/* Never do this initialization again. */
	once = 1;
	return (1);
}

#ifdef notyet
void
read_names(void)
{
	struct disklist_head disk_head;
	struct dkdevice	cur_disk, *p;

	char two_char[2];
	register int i;

	kread(X_DK_NAME, &dk_name, dk_ndrive * sizeof(char *));
	kread(X_DK_UNIT, &dk_unit, dk_ndrive * sizeof(int));

	p = dk_drivehead;
	for (i = 0; dk_ndrive; i++) {
		kread2(p, &cur_disk, sizeof(cur_disk));
		dk_name[i] = &cur_disk.dk_name;
		kread2(dk_name[i], two_char, sizeof(two_char));
		sprintf(dr_name[i], "%c%c%d", two_char[0], two_char[1], dk_unit[i]);
		p = TAILQ_NEXT(cur_disk, dk_link);
	}
}
#endif

/*
 * kread reads something from the kernel, given its nlist index.
 */
static void
kread(nlx, addr, size)
	int nlx;
	void *addr;
	size_t size;
{
	kread2(nlx, addr, size);
}

static void
kread2(ptr, addr, size)
	void *ptr, *addr;
	size_t size;
{
	char *sym;
	char buf[128];
	int nlx;

	nlx = (int)ptr;
	if (isdigit(nlx) == 0) {
		if (namelist[nlx].n_type == 0 || namelist[nlx].n_value == 0) {
			sym = namelist[nlx].n_name;
			if (*sym == '_') {
				++sym;
			}
			(void)fprintf(stderr, "vmstat: symbol %s not defined\n", sym);
			exit(1);
		}
		if (kvm_read(kd, namelist[nlx].n_value, addr, size) != size) {
			sym = namelist[nlx].n_name;
			if (*sym == '_') {
				++sym;
			}
			(void)fprintf(stderr, "vmstat: %s: %s\n", sym, kvm_geterr(kd));
			exit(1);
		}
	} else {
		if (kvm_read(kd, (u_long)ptr, (char *)addr, size) != size) {
			memset(buf, 0, sizeof(buf));
			(void)fprintf(stderr, "vmstat: can't dereference ptr 0x%lx: %s: %s\n", (u_long)ptr, buf, kvm_geterr(kd));
			exit(1);
		}
	}
}

static void
usage()
{
	(void)fprintf(stderr, "usage: vmstat [-fimst] [-c count] [-M core] [-N system] [-w wait] [disks]\n");
	exit(1);
}
