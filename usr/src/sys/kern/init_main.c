/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)init_main.c	2.5 (2.11BSD GTE) 1997/9/26
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/resourcevar.h>
#include <sys/stat.h>
#include <sys/sysent.h>
#include <sys/user.h>

#include <machine/cpu.h>

#include <vm/vm.h>

int	netoff = 1;
int cmask = CMASK;
int	securelevel;

struct proc proc0;
struct proc *curproc = &proc0;

extern	size_t physmem;
extern	struct mapent _coremap[];


/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 */
int
main()
{
	extern dev_t bootdev;
	extern caddr_t bootcsr;
	register struct proc *p;
	register struct user *u;
	extern struct sysentvec aout_sysvec;
	register int i;
	register struct fs *fs;
	time_t  toytime, toyclk();
	daddr_t swsize;
	int	(*ioctl)();
	struct	partinfo dpart;

	register struct bdevsw bdevsw;
	register struct cdevsw cdevsw;

	/*
	 * Initialize the current process pointer (curproc) before
	 * any possible traps/probes to simplify trap processing.
	 */

	p = &proc0;
	curproc = p;

	startup();

	/*
	 * set up system process 0 (swapper)
	 */

	p->p_sysent = &aout_sysvec;

	p->p_stat = SRUN;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;

	u->u_procp = p;			/* init user structure */
	u->u_ap = u->u_arg;
	u->u_cmask = cmask;
	u->u_lastfile = -1;
	for (i = 1; i < NGROUPS; i++)
		u->u_groups[i] = NOGROUP;
	for (i = 0; i < sizeof(u->u_rlimit)/sizeof(u->u_rlimit[0]); i++)
		u->u_rlimit[i]->rlim_cur = u->u_rlimit[i].rlim_max =
		    RLIM_INFINITY;
	bcopy("root", u->u_login, sizeof ("root"));

	/* Initialize signal state for process 0 */
	siginit(p);

	/*
	 * Initialize tables, protocols, and set up well-known inodes.
	 */
	cinit();
	pqinit();
	xinit();
	ihinit();
	bhinit();
	binit();
	ubinit();
	nchinit();
	clkstart();

/*
 * If the kernel is configured for the boot/load device AND the use of the
 * compiled in 'bootdev' has not been overridden (by turning on RB_DFLTROOT,
 * see conf/boot.c for details) THEN switch 'rootdev', 'swapdev' and 'pipedev'
 * over to the boot/load device.  Set 'pipedev' to be 'rootdev'.
 *
 * The &077 removes the controller number (bits 6 and 7) - those bits are 
 * passed thru from /boot but would only greatly confuse the rest of the kernel.
*/
	i = major(bootdev);
	if	((bdevsw[i].d_strategy != nodev) && !(boothowto & RB_DFLTROOT))
		{
		rootdev = makedev(i, minor(bootdev) & 077);
		swapdev = rootdev | 1;	/* partition 'b' */
		pipedev = rootdev;
/*
 * We check that the dump device is the same as the boot device.  If it is 
 * different then it is likely that crashdumps go to a tape device rather than 
 * the swap area.  In that case do not switch the dump device.
*/
		if	((dumpdev != NODEV) && major(dumpdev) == i)
			dumpdev = swapdev;
		}

/*
 * Need to attach the root device.  The CSR is passed thru because this
 * may be a 2nd or 3rd controller rather than the 1st.  NOTE: This poses
 * a big problem if 'swapdev' is not on the same controller as 'rootdev'
 * _or_ if 'swapdev' itself is on a 2nd or 3rd controller.  Short of moving
 * autconfigure back in to the kernel it is not known what can be done about
 * this.
 *
 * One solution (for now) is to call swapdev's attach routine with a zero
 * address.  The MSCP driver treats the 0 as a signal to perform the
 * old (fixed address) attach.  Drivers (all the rest at this point) which
 * do not support alternate controller booting always attach the first
 * (primary) CSR and do not expect an argument to be passed.
*/
	(void)(*bdevsw[major(bootdev)].d_root)(bootcsr);
	(void)(*bdevsw[major(swapdev)].d_root)((caddr_t) 0);	/* XXX */

/*
 * Now we find out how much swap space is available.  Since 'nswap' is
 * a "u_int" we have to restrict the amount of swap to 65535 sectors (~32mb).
 * Considering that 4mb is the maximum physical memory capacity of a pdp-11
 * 32mb swap should be enough ;-)
 *
 * The initialization of the swap map was moved here from machdep2.c because
 * 'nswap' was no longer statically defined and this is where the swap dev
 * is opened/initialized.
 *
 * Also, we toss away/ignore .5kb (1 sector) of swap space (because a 0 value
 * can not be placed in a resource map).
 *
 * 'swplo' was a hack which has _finally_ gone away!  It was never anything
 * but 0 and caused a number of double word adds in the kernel.
*/
	(*bdevsw[major(swapdev)].d_open)(swapdev, FREAD|FWRITE, S_IFBLK);
	swsize = (*bdevsw[major(swapdev)].d_psize)(swapdev);
	if	(swsize <= 0)
		panic("swsiz");		/* don't want to panic, but what ? */
/*
 * Next we make sure that we do not swap on a partition unless it is of
 * type FS_SWAP.  If the driver does not have an ioctl entry point or if
 * retrieving the partition information fails then the driver does not 
 * support labels and we proceed normally, otherwise the partition must be
 * a swap partition (so that we do not swap on top of a filesystem by mistake).
*/
	ioctl = cdevsw[blktochr(swapdev)].d_ioctl;
	if	(ioctl && !(*ioctl)(swapdev, DIOCGPART, (caddr_t)&dpart, FREAD))
		{
		if	(dpart.part->p_fstype != FS_SWAP)
			panic("swtyp");
		}
	if	(swsize > (daddr_t)65535)
		swsize = 65535;
	nswap = swsize;
	mfree(swapmap, --nswap, 1);

	fs = mountfs(rootdev, boothowto & RB_RDONLY ? MNT_RDONLY : 0,
			(struct inode *)0);
	if (!fs)
		panic("iinit");
	mount[0].m_inodp = (struct inode *)1;	/* XXX */
	mount_updname(fs, "/", "root", 1, 4);
	time.tv_sec = fs->fs_time;
	if	(toytime = toyclk())
		time.tv_sec = toytime;
	boottime = time;

/* kick off timeout driven events by calling first time */
	schedcpu();

/* set up the root file system */
	rootdir = iget(rootdev, &mount[0].m_filsys, (ino_t)ROOTINO);
	iunlock(rootdir);
	u->u_cdir = iget(rootdev, &mount[0].m_filsys, (ino_t)ROOTINO);
	iunlock(u->u_cdir);
	u->u_rdir = NULL;


/*
 * This came from pdp/machdep2.c because the memory available statements
 * were being made _before_ memory for the networking code was allocated.
 * A side effect of moving this code is that network "attach" and MSCP 
 * "online" messages can appear before the memory sizes.  The (currently
 * safe) assumption is made that no 'free' calls are made so that the
 * size in the first entry of the core map is correct.
*/
	printf("\nphys mem  = %D\n", ctob((long)physmem));
	printf("avail mem = %D\n", ctob((long)_coremap[0].m_size));
	maxmem = MAXMEM;
	printf("user mem  = %D\n", ctob((long)MAXMEM));
#if NRAM > 0
	printf("ram disk  = %D\n", ctob((long)ramsize));
#endif
	printf("\n");

	/*
	 * make init process
	 */
	if (newproc(0)) {
		expand((int)btoc(szicode), S_DATA);
		expand((int)1, S_STACK);	/* one click of stack */
		estabur((u_int)0, (u_int)btoc(szicode), (u_int)1, 0, RO);
		copyout((caddr_t)icode, (caddr_t)0, szicode);
		/*
		 * return goes to location 0 of user init code
		 * just copied out.
		 */
		return 0;
	}
	else
		sched();
}

/*
 * Initialize hash links for buffers.
 */
static void
bhinit()
{
	register int i;
	register struct bufhd *bp;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

memaddr	bpaddr;		/* physical click-address of buffers */ /* pdp-11 unibus ref */
/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
static void
binit()
{
	register struct buf *bp;
	register int i;
	long paddr;

	for (bp = bfreelist; bp < &bfreelist[BQUEUES]; bp++)
		bp->b_forw = bp->b_back = bp->av_forw = bp->av_back = bp;
	paddr = ((long)bpaddr) << 6;									/* pdp-11 seg.h ref */
	for (i = 0; i < nbuf; i++, paddr += MAXBSIZE) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		bp->b_un.b_addr = (caddr_t)loint(paddr);
		bp->b_xmem = hiint(paddr);
		binshash(bp, &bfreelist[BQ_AGE]);
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
static void
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int)cfree;
	ccp = (ccp + CROUND) & ~CROUND;
#endif
	for (cp = (struct cblock *)ccp; cp <= &cfree[nclist - 1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}

}
