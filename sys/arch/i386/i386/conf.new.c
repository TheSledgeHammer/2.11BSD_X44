/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)conf.c	3.2 (2.11BSD GTE) 1997/11/12
 */
/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)conf.c	8.3 (Berkeley) 1/21/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/clist.h>
#include <sys/vnode.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>

//#include <dev/conf_decl.h>
#include <machine/conf.h>

#include "wd.h"
#include "fd.h"
#include "wt.h"
#include "xd.h"
#include "vnd.h"
#include "pty.h"
#include "com.h"
#include "pc.h"
#include "bpfilter.h"

#define	pts_tty		pt_tty
#define	ptsioctl	ptyioctl
#define	ptc_tty		pt_tty
#define	ptcioctl	ptyioctl

struct bdevsw bdevsw[] = {
	bdev_disk_init(NWD,wd),			/* 0: st506/rll/esdi/ide disk */
	bdev_swap_init(1,sw),			/* 1: swap pseudo-device */
	bdev_disk_init(NFD,Fd),			/* 2: floppy disk */
	bdev_tape_init(NWT,wt),			/* 3: QIC-24/60/120/150 cartridge tape */
	bdev_disk_init(NXD,xd),			/* 4: temp alt st506/rll/esdi/ide disk */
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

struct cdevsw cdevsw[] = {
	cdev_cn_init(1,cn),				/* 0: virtual console */
	cdev_ctty_init(1,ctty),			/* 1: controlling terminal */
	cdev_mm_init(1,mm),				/* 2: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NWD,wd),			/* 3: st506/rll/esdi/ide disk */
	cdev_swap_init(1,sw),			/* 4: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),		/* 5: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),		/* 6: pseudo-tty master */
	cdev_log_init(1,log),			/* 7: /dev/klog */
	cdev_tty_init(NCOM,com),		/* 8: serial communications ports */
	cdev_disk_init(NFD,Fd),			/* 9: floppy disk */
	cdev_tape_init(NWT,wt),			/* 10: QIC-24/60/120/150 cartridge tape */
	cdev_disk_init(NXD,xd),			/* 11: temp alt st506/rll/esdi/ide disk */
	cdev_pc_init(1,pc),				/* 12: real console */
	cdev_notdef(),					/* 13 */
	cdev_bpf_init(NBPFILTER,bpf),	/* 14: berkeley packet filter */
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
int
iskmemdev(dev)
	register dev_t dev;
{
	if (major(dev) == 2 && (minor(dev) == 0 || minor(dev) == 1))
		return (1);
	return (0);
}

int
iszerodev(dev)
	dev_t dev;
{
	return (major(dev) == 2 && minor(dev) == 12);
}

/*
 * Routine to determine if a device is a disk.
 *
 * A minimal stub routine can always return 0.
 */
int
isdisk(dev, type)
	dev_t dev;
	register int type;
{
	switch (major(dev)) {

	default:
		return (0);
	}
	/* NOTREACHED */
}

/* TODO: Add devices */
#define MAXDEV 31
static char chrtoblktbl[MAXDEV] =  {
		/* BLK */	/* CHR */
		NODEV,		/* 0 */
		NODEV,		/* 1 */
		NODEV,		/* 2 */
		NODEV,		/* 3 */
		NODEV,		/* 4 */
		NODEV,		/* 5 */
		NODEV,		/* 6 */
		NODEV,		/* 7 */
		NODEV,		/* 8 */
		NODEV,		/* 9 */
		NODEV,		/* 10 */
		NODEV,		/* 11 */
		NODEV,		/* 12 */
		NODEV,		/* 13 */
		NODEV,		/* 14 */
		NODEV,		/* 15 */
		NODEV,		/* 16 */
		NODEV,		/* 17 */
		NODEV,		/* 18 */
		NODEV,		/* 19 */
		NODEV,		/* 20 */
		NODEV,		/* 21 */
		NODEV,		/* 22 */
		NODEV,		/* 23 */
		NODEV,		/* 24 */
		NODEV,		/* 25 */
		NODEV,		/* 26 */
		NODEV,		/* 27 */
		NODEV,		/* 28 */
		NODEV,		/* 29 */
		NODEV,		/* 30 */
};

/*
 * Routine to convert from character to block device number.
 *
 * A minimal stub routine can always return NODEV.
 */
int
chrtoblk(dev)
	register dev_t dev;
{
	register int blkmaj;

	if (major(dev) >= MAXDEV || (blkmaj = chrtoblktbl[major(dev)]) == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}

/*
 * This routine returns the cdevsw[] index of the block device
 * specified by the input parameter.    Used by init_main and ufs_mount to
 * find the diskdriver's ioctl entry point so that the label and partition
 * information can be obtained for 'block' (instead of 'character') disks.
 *
 * Rather than create a whole separate table 'chrtoblktbl' is scanned
 * looking for a match.  This routine is only called a half dozen times during
 * a system's life so efficiency isn't a big concern.
*/
int
blktochr(dev)
	register dev_t dev;
{
	register int maj = major(dev);
	register int i;

	for	(i = 0; i < MAXDEV; i++) {
		if	(maj == chrtoblktbl[i])
			return(i);
	}
	return(NODEV);
}
