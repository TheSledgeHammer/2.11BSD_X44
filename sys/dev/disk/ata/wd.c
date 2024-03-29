/*	$NetBSD: wd.c,v 1.274.2.8.2.6.2.6 2007/10/15 23:10:19 riz Exp $ */

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *	derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998, 2003, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wd.c,v 1.274.2.8.2.6.2.6 2007/10/15 23:10:19 riz Exp $");

#ifndef WDCDEBUG
#define WDCDEBUG
#endif /* WDCDEBUG */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/power.h>
#include <sys/ataio.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/disk/ata/atareg.h>
#include <dev/disk/ata/atavar.h>
#include <dev/disk/ata/wdvar.h>
#include <dev/core/io/wdc/wdcreg.h>

#include "locators.h"

#define	LBA48_THRESHOLD				(0xfffffff)	/* 128GB / DEV_BSIZE */

#define	WDIORETRIES_SINGLE 			4			/* number of retries before single-sector */
#define	WDIORETRIES					5			/* number of retries before giving up */
#define	RECOVERYTIME 				hz/2		/* time to wait before retrying a cmd */

#define	WDUNIT(dev)					dkunit(dev)
#define	WDPART(dev)					dkpart(dev)
#define	WDMINOR(unit, part)			dkminor(unit, part)
#define	MAKEWDDEV(maj, unit, part)	dkmakedev(maj, unit, part)

#define	WDLABELDEV(dev)				(MAKEWDDEV(major(dev), WDUNIT(dev), RAW_PART))

#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#ifdef WDCDEBUG
int wdcdebug_wd_mask = 0x0; 
#define WDCDEBUG_PRINT(args, level) \
	if (wdcdebug_wd_mask & (level)) \
		printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif

int		wdprobe(struct device *, struct cfdata *, void *);
void 	wdattach(struct device *, struct device *, void *);
int		wddetach(struct device *, int);
int		wdactivate(struct device *, enum devact);
int		wdprint(void *, char *);
void 	wdperror(const struct wd_softc *);

CFOPS_DECL(wd, wdprobe, wdattach, wddetach, wdactivate);
CFDRIVER_DECL(NULL, wd, DV_DISK);
CFATTACH_DECL(wd, &wd_cd, &wd_cops, sizeof(struct wd_softc));

extern struct cfdriver wd_cd;

dev_type_open(wdopen);
dev_type_close(wdclose);
dev_type_read(wdread);
dev_type_write(wdwrite);
dev_type_ioctl(wdioctl);
dev_type_strategy(wdstrategy);
dev_type_dump(wddump);
dev_type_size(wdsize);

const struct bdevsw wd_bdevsw = {
	.d_open = wdopen,
	.d_close = wdclose,
	.d_strategy = wdstrategy,
	.d_ioctl = wdioctl,
	.d_dump = wddump,
	.d_psize = wdsize,
	.d_type = D_DISK
};

const struct cdevsw wd_cdevsw = {
	.d_open = wdopen,
	.d_close = wdclose,
	.d_read = wdread,
	.d_write = wdwrite,
	.d_ioctl = wdioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_type = D_DISK
};

/*
 * Glue necessary to hook WDCIOCCOMMAND into physio
 */

struct wd_ioctl {
	LIST_ENTRY(wd_ioctl) wi_list;
	struct buf wi_bp;
	struct uio wi_uio;
	struct iovec wi_iov;
	atareq_t wi_atareq;
	struct wd_softc *wi_softc;
};

LIST_HEAD(, wd_ioctl) wi_head;

struct	wd_ioctl *wi_find(struct buf *);
void	wi_free(struct wd_ioctl *);
struct	wd_ioctl *wi_get(void);
void	wdioctlstrategy(struct buf *);

void  wdgetdefaultlabel(struct wd_softc *, struct disklabel *);
void  wdgetdisklabel(struct wd_softc *);
void  wdstart(void *);
void  __wdstart(struct wd_softc*, struct buf *);
void  wdrestart(void *);
void  wddone(void *);

#ifdef B_FORMAT
int   wdformat(struct buf *);
#endif

int   wd_get_params(struct wd_softc *, u_int8_t, struct ataparams *);
int   wd_flushcache(struct wd_softc *, int);
void  wd_shutdown(void *);

int   wd_getcache(struct wd_softc *, int *);
int   wd_setcache(struct wd_softc *, int);

struct dkdriver wddkdriver = { wdstrategy };

#ifdef HAS_BAD144_HANDLING
static void bad144intern(struct wd_softc *);
#endif

#define	WD_QUIRK_SPLIT_MOD15_WRITE	0x0001	/* must split certain writes */
#define	WD_QUIRK_FORCE_LBA48		0x0002	/* must use LBA48 commands */

/*
 * Quirk table for IDE drives.  Put more-specific matches first, since
 * a simple globbing routine is used for matching.
 */
static const struct wd_quirk {
	const char *wdq_match;		/* inquiry pattern to match */
	int wdq_quirks;			/* drive quirks */
} wd_quirk_table[] = {
	/*
	 * Some Seagate S-ATA drives have a PHY which can get confused
	 * with the way data is packetized by some S-ATA controllers.
	 *
	 * The work-around is to split in two any write transfer whose
	 * sector count % 15 == 1 (assuming 512 byte sectors).
	 *
	 * XXX This is an incomplete list.  There are at least a couple
	 * XXX more model numbers.  If you have trouble with such transfers
	 * XXX (8K is the most common) on Seagate S-ATA drives, please
	 * XXX notify thorpej@NetBSD.org.
	 */
	{ "ST3120023AS",
	  WD_QUIRK_SPLIT_MOD15_WRITE },
	{ "ST380023AS",
	  WD_QUIRK_SPLIT_MOD15_WRITE },

	/*
	 * This seagate drive seems to have issue addressing sector 0xfffffff
	 * (aka LBA48_THRESHOLD) in LBA mode. The workaround is to force
	 * LBA48
	 */
	{ "ST3160021A*",
	  WD_QUIRK_FORCE_LBA48 },
	{ "ST3160811A*",
	  WD_QUIRK_FORCE_LBA48 },
	{ "ST3160812A*",
	  WD_QUIRK_FORCE_LBA48 },
	{ "ST3160023A*",
	  WD_QUIRK_FORCE_LBA48 },
	{ "ST3160827A*",
	  WD_QUIRK_FORCE_LBA48 },
	/* Attempt to catch all seagate drives larger than 200GB */
	{ "ST3[2-9][0-9][0-9][0-9][0-9][0-9][A-Z]*",
	  WD_QUIRK_FORCE_LBA48 },
	{ NULL,
	  0 }
};

static const struct wd_quirk *
wd_lookup_quirks(const char *name)
{
	const struct wd_quirk *wdq;
	const char *estr;

	for (wdq = wd_quirk_table; wdq->wdq_match != NULL; wdq++) {
		/*
		 * We only want exact matches (which include matches
		 * against globbing characters).
		 */
		if (pmatch(name, wdq->wdq_match, &estr) == 2)
			return (wdq);
	}
	return (NULL);
}

int
wdprobe(struct device *parent, struct cfdata *match, void *aux)
{
	struct ata_device *adev = aux;

	if (adev == NULL)
		return 0;
	if (adev->adev_bustype->bustype_type != SCSIPI_BUSTYPE_ATA)
		return 0;

	if (match->cf_loc[ATA_HLCF_DRIVE] != ATA_HLCF_DRIVE_DEFAULT &&
	    match->cf_loc[ATA_HLCF_DRIVE] != adev->adev_drv_data->drive)
		return 0;
	return 1;
}

void
wdattach(struct device *parent, struct device *self, void *aux)
{
	struct wd_softc *wd = (void *)self;
	struct ata_device *adev= aux;
	int i, blank;
	char buf[41], pbuf[9], c, *p, *q;
	const struct wd_quirk *wdq;
	WDCDEBUG_PRINT(("wdattach\n"), DEBUG_FUNCS | DEBUG_PROBE);

	lockinit(&wd->sc_lock, PRIBIO | PCATCH, "wdlock", 0, 0);

	callout_init(&wd->sc_restart_ch);
	bufq_alloc(&wd->sc_q, BUFQ_DISK_DEFAULT_STRAT() | BUFQ_SORT_RAWBLOCK);
	SLIST_INIT(&wd->sc_bslist);

	wd->atabus = adev->adev_bustype;
	wd->openings = adev->adev_openings;
	wd->drvp = adev->adev_drv_data;

	wd->drvp->drv_done = wddone;
	wd->drvp->drv_softc = &wd->sc_dev;

	printf("\n");

	/* read our drive info */
	if (wd_get_params(wd, AT_WAIT, &wd->sc_params) != 0) {
		printf("\n%s: IDENTIFY failed\n", wd->sc_dev.dv_xname);
		return;
	}

	for (blank = 0, p = wd->sc_params.atap_model, q = buf, i = 0;
	    i < sizeof(wd->sc_params.atap_model); i++) {
		c = *p++;
		if (c == '\0')
			break;
		if (c != ' ') {
			if (blank) {
				*q++ = ' ';
				blank = 0;
			}
			*q++ = c;
		} else
			blank = 1;
	}
	*q++ = '\0';

	printf(": <%s>\n", buf);

	wdq = wd_lookup_quirks(buf);
	if (wdq != NULL)
		wd->sc_quirks = wdq->wdq_quirks;

	if ((wd->sc_params.atap_multi & 0xff) > 1) {
		wd->sc_multi = wd->sc_params.atap_multi & 0xff;
	} else {
		wd->sc_multi = 1;
	}

	printf("%s: drive supports %d-sector PIO transfers,",
	    wd->sc_dev.dv_xname, wd->sc_multi);

	/* 48-bit LBA addressing */
	if ((wd->sc_params.atap_cmd2_en & ATA_CMD2_LBA48) != 0)
		wd->sc_flags |= WDF_LBA48;

	/* Prior to ATA-4, LBA was optional. */
	if ((wd->sc_params.atap_capabilities1 & WDC_CAP_LBA) != 0)
		wd->sc_flags |= WDF_LBA;
#if 0
	/* ATA-4 requires LBA. */
	if (wd->sc_params.atap_ataversion != 0xffff &&
	    wd->sc_params.atap_ataversion >= WDC_VER_ATA4)
		wd->sc_flags |= WDF_LBA;
#endif

	if ((wd->sc_flags & WDF_LBA48) != 0) {
		printf(" LBA48 addressing\n");
		wd->sc_capacity =
		    ((u_int64_t) wd->sc_params.__reserved6[11] << 48) |
		    ((u_int64_t) wd->sc_params.__reserved6[10] << 32) |
		    ((u_int64_t) wd->sc_params.__reserved6[9]  << 16) |
		    ((u_int64_t) wd->sc_params.__reserved6[8]  << 0);
	} else if ((wd->sc_flags & WDF_LBA) != 0) {
		printf(" LBA addressing\n");
		wd->sc_capacity =
		    (wd->sc_params.atap_capacity[1] << 16) |
		    wd->sc_params.atap_capacity[0];
	} else {
		printf(" chs addressing\n");
		wd->sc_capacity =
		    wd->sc_params.atap_cylinders *
		    wd->sc_params.atap_heads *
		    wd->sc_params.atap_sectors;
	}
	//format_bytes(pbuf, sizeof(pbuf), wd->sc_capacity * DEV_BSIZE);
	printf("%s: %s, %d cyl, %d head, %d sec, "
			"%d bytes/sect x %llu sectors\n", self->dv_xname, pbuf,
			(wd->sc_flags & WDF_LBA) ?
					(int) (wd->sc_capacity
							/ (wd->sc_params.atap_heads
									* wd->sc_params.atap_sectors)) :
					wd->sc_params.atap_cylinders, wd->sc_params.atap_heads,
			wd->sc_params.atap_sectors, DEV_BSIZE,
			(unsigned long long) wd->sc_capacity);

	WDCDEBUG_PRINT(
			("%s: atap_dmatiming_mimi=%d, atap_dmatiming_recom=%d\n", self->dv_xname, wd->sc_params.atap_dmatiming_mimi, wd->sc_params.atap_dmatiming_recom),
			DEBUG_PROBE);
	/*
	 * Initialize and attach the disk structure.
	 */
	wd->sc_dk.dk_driver = &wddkdriver;
	wd->sc_dk.dk_name = wd->sc_dev.dv_xname;
	disk_attach(&wd->sc_dk, &wd_bdevsw, &wd_cdevsw);
	wd->sc_wdc_bio.lp = wd->sc_dk.dk_label;
	wd->sc_sdhook = shutdownhook_establish(wd_shutdown, wd);
	if (wd->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
				wd->sc_dev.dv_xname);
}

int
wdactivate(struct device *self, enum devact act)
{
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVATE.
		 */
		break;
	}
	return (rv);
}

int
wddetach(struct device *self, int flags)
{
	struct wd_softc *sc = (struct wd_softc *)self;
	struct buf *bp;
	int s, bmaj, cmaj, i, mn;

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&wd_bdevsw);
	cmaj = cdevsw_lookup_major(&wd_cdevsw);

	s = splbio();

	/* Kill off any queued buffers. */
	while ((bp = BUFQ_GET(&sc->sc_q)) != NULL) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}

	bufq_free(&sc->sc_q);

	splx(s);

	/* Nuke the vnodes for any open instances. */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = WDMINOR(self->dv_unit, i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Detach disk. */
	disk_detach(&sc->sc_dk);

	/* Clean out the bad sector list */
	while (!SLIST_EMPTY(&sc->sc_bslist)) {
		void *head = SLIST_FIRST(&sc->sc_bslist);
		SLIST_REMOVE_HEAD(&sc->sc_bslist, dbs_next);
		free(head, M_TEMP);
	}
	sc->sc_bscount = 0;

	/* Get rid of the shutdown hook. */
	if (sc->sc_sdhook != NULL)
		shutdownhook_disestablish(sc->sc_sdhook);

	lockmgr(&sc->sc_lock, LK_DRAIN, &sc->sc_lock.lk_lnterlock, LOCKHOLDER_PID(&sc->sc_lock.lk_lockholder));

	return (0);
}

/*
 * Read/write routine for a buffer.  Validates the arguments and schedules the
 * transfer.  Does not wait for the transfer to complete.
 */
void
wdstrategy(struct buf *bp)
{
	struct wd_softc *wd = wd_cd.cd_devs[WDUNIT(bp->b_dev)];
	struct disklabel *lp = wd->sc_dk.dk_label;
	daddr_t blkno;
	int s;

	WDCDEBUG_PRINT(("wdstrategy (%s)\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);

	/* Valid request?  */
	if (bp->b_blkno < 0 ||
	    (bp->b_bcount % lp->d_secsize) != 0 ||
	    (bp->b_bcount / lp->d_secsize) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		goto bad;
	}

	/* If device invalidated (e.g. media change, door open), error. */
	if ((wd->sc_flags & WDF_LOADED) == 0) {
		bp->b_error = EIO;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (WDPART(bp->b_dev) == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    wd->sc_capacity) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&wd->sc_dk, bp,
		    (wd->sc_flags & (WDF_WLABEL|WDF_LABELLING)) != 0) <= 0)
			goto done;
	}

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	if (lp->d_secsize >= DEV_BSIZE)
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		blkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);

	if (WDPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[WDPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	/*
	 * If the transfer about to be attempted contains only a block that
	 * is known to be bad then return an error for the transfer without
	 * even attempting to start a transfer up under the premis that we
	 * will just end up doing more retries for a transfer that will end
	 * up failing again.
	 * XXX:SMP - mutex required to protect with DIOCBSFLUSH
	 */
	if (__predict_false(!SLIST_EMPTY(&wd->sc_bslist))) {
		struct dkbadsectors *dbs;
		daddr_t maxblk = blkno + (bp->b_bcount >> DEV_BSHIFT) - 1;

		SLIST_FOREACH(dbs, &wd->sc_bslist, dbs_next)
			if ((dbs->dbs_min <= blkno && blkno <= dbs->dbs_max) ||
			    (dbs->dbs_min <= maxblk && maxblk <= dbs->dbs_max)){
				bp->b_error = EIO;
				goto bad;
			}
	}

	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	BUFQ_PUT(&wd->sc_q, bp);
	wdstart(wd);
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	/* Toss transfer; we're done early. */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * Queue a drive for I/O.
 */
void
wdstart(void *arg)
{
	struct wd_softc *wd = arg;
	struct buf *bp = NULL;

	WDCDEBUG_PRINT(("wdstart %s\n", wd->sc_dev.dv_xname), DEBUG_XFERS);
	while (wd->openings > 0) {

		/* Is there a buf for us ? */
		if ((bp = BUFQ_GET(&wd->sc_q)) == NULL)
			return;

		/*
		 * Make the command. First lock the device
		 */
		wd->openings--;

		wd->retries = 0;
		__wdstart(wd, bp);
	}
}

static void
wd_split_mod15_write(struct buf *bp)
{
	struct buf *obp = bp->b_private;
	struct wd_softc *sc = wd_cd.cd_devs[dkunit(obp->b_dev)];

	if (__predict_false(bp->b_flags & B_ERROR) != 0) {
		/*
		 * Propagate the error.  If this was the first half of
		 * the original transfer, make sure to account for that
		 * in the residual.
		 */
		if (bp->b_data == obp->b_data)
			bp->b_resid += bp->b_bcount;
		goto done;
	}

	/*
	 * If this was the second half of the transfer, we're all done!
	 */
	if (bp->b_data != obp->b_data)
		goto done;

	/*
	 * Advance the pointer to the second half and issue that command
	 * using the same opening.
	 */
	bp->b_flags = obp->b_flags | B_CALL;
	bp->b_data += bp->b_bcount;
	bp->b_blkno += (bp->b_bcount / 512);
	bp->b_rawblkno += (bp->b_bcount / 512);
	__wdstart(sc, bp);
	return;

 done:
	obp->b_flags |= (bp->b_flags & (B_EINTR|B_ERROR));
	obp->b_error = bp->b_error;
	obp->b_resid = bp->b_resid;

	free(bp, M_DEVBUF);
	biodone(obp);
	sc->openings++;
	/* wddone() will call wdstart() */
}

void
__wdstart(struct wd_softc *wd, struct buf *bp)
{

	/*
	 * Deal with the "split mod15 write" quirk.  We just divide the
	 * transfer in two, doing the first half and then then second half
	 * with the same command opening.
	 *
	 * Note we MUST do this here, because we can't let insertion
	 * into the bufq cause the transfers to be re-merged.
	 */
	if (__predict_false(
			(wd->sc_quirks & WD_QUIRK_SPLIT_MOD15_WRITE) != 0
					&& (bp->b_flags & B_READ) == 0 && bp->b_bcount > 512
					&& ((bp->b_bcount / 512) % 15) == 1)) {
		struct buf *nbp;

		/* already at splbio */
		nbp = (struct buf *)malloc(sizeof(struct buf *), M_DEVBUF, M_NOWAIT);
		if (__predict_false(nbp == NULL)) {
			/* No memory -- fail the iop. */
			bp->b_error = ENOMEM;
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			wd->openings++;
			return;
		}

		BUF_INIT(nbp);
		nbp->b_error = 0;
		nbp->b_proc = bp->b_proc;
		nbp->b_vp = NULLVP;
		nbp->b_dev = bp->b_dev;

		nbp->b_bcount = bp->b_bcount / 2;
		nbp->b_bufsize = bp->b_bcount / 2;
		nbp->b_data = bp->b_data;

		nbp->b_blkno = bp->b_blkno;
		nbp->b_rawblkno = bp->b_rawblkno;

		nbp->b_flags = bp->b_flags | B_CALL;
		nbp->b_iodone = wd_split_mod15_write;

		/* Put ptr to orig buf in b_private and use new buf */
		nbp->b_private = bp;

		BIO_COPYPRIO(nbp, bp);

		bp = nbp;
	}

	wd->sc_wdc_bio.blkno = bp->b_rawblkno;
	wd->sc_wdc_bio.blkdone = 0;
	wd->sc_bp = bp;
	/*
	 * If we're retrying, retry in single-sector mode. This will give us
	 * the sector number of the problem, and will eventually allow the
	 * transfer to succeed.
	 */
	if (wd->retries >= WDIORETRIES_SINGLE)
		wd->sc_wdc_bio.flags = ATA_SINGLE;
	else
		wd->sc_wdc_bio.flags = 0;
	if ((wd->sc_flags & WDF_LBA48)
			&& (wd->sc_wdc_bio.blkno > LBA48_THRESHOLD
					|| (wd->sc_quirks & WD_QUIRK_FORCE_LBA48) != 0))
		wd->sc_wdc_bio.flags |= ATA_LBA48;
	if (wd->sc_flags & WDF_LBA)
		wd->sc_wdc_bio.flags |= ATA_LBA;
	if (bp->b_flags & B_READ)
		wd->sc_wdc_bio.flags |= ATA_READ;
	wd->sc_wdc_bio.bcount = bp->b_bcount;
	wd->sc_wdc_bio.databuf = bp->b_data;
	/* Instrumentation. */
	disk_busy(&wd->sc_dk);
	switch (wd->atabus->ata_bio(wd->drvp, &wd->sc_wdc_bio)) {
	case WDC_TRY_AGAIN:
		callout_reset(&wd->sc_restart_ch, hz, wdrestart, wd);
		break;
	case WDC_QUEUED:
	case WDC_COMPLETE:
		break;
	default:
		panic("__wdstart: bad return code from ata_bio()");
	}
}

void
wddone(void *v)
{
	struct wd_softc *wd = v;
	struct buf *bp = wd->sc_bp;
	const char *errmsg;
	int do_perror = 0;
	WDCDEBUG_PRINT(("wddone %s\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);
	int nblks;

	if (bp == NULL)
		return;
	bp->b_resid = wd->sc_wdc_bio.bcount;
	switch (wd->sc_wdc_bio.error) {
	case ERR_DMA:
		errmsg = "DMA error";
		goto retry;
	case ERR_DF:
		errmsg = "device fault";
		goto retry;
	case TIMEOUT:
		errmsg = "device timeout";
		goto retry;
	case ERR_RESET:
		errmsg = "channel reset";
		goto retry2;
	case ERROR:
		/* Don't care about media change bits */
		if (wd->sc_wdc_bio.r_error != 0 &&
		    (wd->sc_wdc_bio.r_error & ~(WDCE_MC | WDCE_MCR)) == 0)
			goto noerror;
		errmsg = "error";
		do_perror = 1;
		if ((wd->sc_wdc_bio.r_error & WDCE_IDNF) &&
		    (wd->sc_quirks & WD_QUIRK_FORCE_LBA48) == 0) {
			nblks = wd->sc_wdc_bio.bcount /
			    wd->sc_dk.dk_label->d_secsize;
			/*
			 * If we get a "id not found" when crossing the
			 * LBA48_THRESHOLD, and the drive is larger than
			 * 128GB, then we can assume the drive has the
			 * LBA48 bug and we switch to LBA48.
			 */
			if (wd->sc_wdc_bio.blkno <= LBA48_THRESHOLD &&
			    wd->sc_wdc_bio.blkno + nblks > LBA48_THRESHOLD &&
			    wd->sc_capacity > LBA48_THRESHOLD + 1) {
				errmsg = "LBA48 bug";
				wd->sc_quirks |= WD_QUIRK_FORCE_LBA48;
				do_perror = 0;
				goto retry2;
			}
		}
retry:		/* Just reset and retry. Can we do more ? */
		wd->atabus->ata_reset_channel(wd->drvp, AT_RST_NOCMD);
retry2:
		diskerr(bp, "wd", errmsg, LOG_PRINTF, wd->sc_wdc_bio.blkdone, wd->sc_dk.dk_label);
		if (wd->retries < WDIORETRIES)
			printf(", retrying");
		printf("\n");
		if (do_perror)
			wdperror(wd);
		if (wd->retries < WDIORETRIES) {
			wd->retries++;
			callout_reset(&wd->sc_restart_ch, RECOVERYTIME,
			    wdrestart, wd);
			return;
		}

		/*
		 * Not all errors indicate a failed block but those that do,
		 * put the block on the bad-block list for the device.  Only
		 * do this for reads because the drive should do it for writes,
		 * itself, according to Manuel.
		 */
		if ((bp->b_flags & B_READ) &&
		    ((wd->drvp->ata_vers >= 4 && (wd->sc_wdc_bio.r_error & 64)) ||
	     	     (wd->drvp->ata_vers < 4 && (wd->sc_wdc_bio.r_error & 192)))) {
			struct dkbadsectors *dbs;

			dbs = malloc(sizeof *dbs, M_TEMP, M_WAITOK);
			dbs->dbs_min = bp->b_rawblkno;
			dbs->dbs_max = dbs->dbs_min + (bp->b_bcount >> DEV_BSHIFT) - 1;
			microtime(&dbs->dbs_failedat);
			SLIST_INSERT_HEAD(&wd->sc_bslist, dbs, dbs_next);
			wd->sc_bscount++;
		}

		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		break;
	case NOERROR:
noerror:	if ((wd->sc_wdc_bio.flags & ATA_CORR) || wd->retries > 0)
			printf("%s: soft error (corrected)\n",
			    wd->sc_dev.dv_xname);
		break;
	case ERR_NODEV:
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		break;
	}
	disk_unbusy(&wd->sc_dk, (bp->b_bcount - bp->b_resid)/*, (bp->b_flags & B_READ)*/);

	/* XXX Yuck, but we don't want to increment openings in this case */
	if (__predict_false((bp->b_flags & B_CALL) != 0 &&
			    bp->b_iodone == wd_split_mod15_write))
		biodone(bp);
	else {
		biodone(bp);
		wd->openings++;
	}
	wdstart(wd);
}

void
wdrestart(void *v)
{
	struct wd_softc *wd = v;
	struct buf *bp = wd->sc_bp;
	int s;
	WDCDEBUG_PRINT(("wdrestart %s\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);

	s = splbio();
	__wdstart(v, bp);
	splx(s);
}

int
wdread(dev_t dev, struct uio *uio, int flags)
{
	WDCDEBUG_PRINT(("wdread\n"), DEBUG_XFERS);
	return (physio(wdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
wdwrite(dev_t dev, struct uio *uio, int flags)
{

	WDCDEBUG_PRINT(("wdwrite\n"), DEBUG_XFERS);
	return (physio(wdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

int
wdopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct wd_softc *wd;
	int part, error;

	WDCDEBUG_PRINT(("wdopen\n"), DEBUG_FUNCS);
	wd = wd_cd.cd_devs[WDUNIT(dev)];
	if (wd == NULL)
		return (ENXIO);

	if ((wd->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	/*
	 * If this is the first open of this device, add a reference
	 * to the adapter.
	 */
	if (wd->sc_dk.dk_openmask == 0 &&
	    (error = wd->atabus->ata_addref(wd->drvp)) != 0)
		return (error);

	if ((error = lockmgr(&wd->sc_lock, LK_EXCLUSIVE, NULL, LOCKHOLDER_PID(&wd->sc_lock.lk_lockholder))) != 0)
		goto bad4;

	if (wd->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((wd->sc_flags & WDF_LOADED) == 0) {
			error = EIO;
			goto bad3;
		}
	} else {
		if ((wd->sc_flags & WDF_LOADED) == 0) {
			wd->sc_flags |= WDF_LOADED;

			/* Load the physical device parameters. */
			wd_get_params(wd, AT_WAIT, &wd->sc_params);

			/* Load the partition info if not already loaded. */
			wdgetdisklabel(wd);
		}
	}

	part = WDPART(dev);

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= wd->sc_dk.dk_label->d_npartitions ||
	     wd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		wd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		wd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	wd->sc_dk.dk_openmask =
	    wd->sc_dk.dk_copenmask | wd->sc_dk.dk_bopenmask;

	lockmgr(&wd->sc_lock, LK_RELEASE, NULL, LOCKHOLDER_PID(&wd->sc_lock.lk_lockholder));
	return 0;

bad:
	if (wd->sc_dk.dk_openmask == 0) {
	}

bad3:
	lockmgr(&wd->sc_lock, LK_RELEASE, NULL, LOCKHOLDER_PID(&wd->sc_lock.lk_lockholder));
bad4:
	if (wd->sc_dk.dk_openmask == 0)
		wd->atabus->ata_delref(wd->drvp);
	return error;
}

int
wdclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct wd_softc *wd = wd_cd.cd_devs[WDUNIT(dev)];
	int part = WDPART(dev);
	int error;

	WDCDEBUG_PRINT(("wdclose\n"), DEBUG_FUNCS);
	if ((error = lockmgr(&wd->sc_lock, LK_EXCLUSIVE, NULL, LOCKHOLDER_PID(&wd->sc_lock.lk_lockholder))) != 0)
		return error;

	switch (fmt) {
	case S_IFCHR:
		wd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		wd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	wd->sc_dk.dk_openmask =
	    wd->sc_dk.dk_copenmask | wd->sc_dk.dk_bopenmask;

	if (wd->sc_dk.dk_openmask == 0) {
		wd_flushcache(wd, AT_WAIT);
		/* XXXX Must wait for I/O to complete! */

		if (! (wd->sc_flags & WDF_KLABEL))
			wd->sc_flags &= ~WDF_LOADED;

		wd->atabus->ata_delref(wd->drvp);
	}

	lockmgr(&wd->sc_lock, LK_RELEASE, NULL, LOCKHOLDER_PID(&wd->sc_lock.lk_lockholder));
	return 0;
}

void
wdgetdefaultlabel(struct wd_softc *wd, struct disklabel *lp)
{

	WDCDEBUG_PRINT(("wdgetdefaultlabel\n"), DEBUG_FUNCS);
	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = DEV_BSIZE;
	lp->d_ntracks = wd->sc_params.atap_heads;
	lp->d_nsectors = wd->sc_params.atap_sectors;
	lp->d_ncylinders = (wd->sc_flags & WDF_LBA) ? wd->sc_capacity /
		(wd->sc_params.atap_heads * wd->sc_params.atap_sectors) :
		wd->sc_params.atap_cylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	if (strcmp(wd->sc_params.atap_model, "ST506") == 0)
		lp->d_type = DTYPE_ST506;
	else
		lp->d_type = DTYPE_ESDI;

	strncpy(lp->d_typename, wd->sc_params.atap_model, 16);
	strncpy(lp->d_packname, "fictitious", 16);
	if (wd->sc_capacity > UINT32_MAX)
		lp->d_secperunit = UINT32_MAX;
	else
		lp->d_secperunit = wd->sc_capacity;
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size =
	    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Fabricate a default disk label, and try to read the correct one.
 */
void
wdgetdisklabel(struct wd_softc *wd)
{
	struct disklabel *lp;
	const char *errstring;

	lp = wd->sc_dk.dk_label;
	WDCDEBUG_PRINT(("wdgetdisklabel\n"), DEBUG_FUNCS);

	wdgetdefaultlabel(wd, lp);

	wd->sc_badsect[0] = -1;

	if (wd->drvp->state > RESET)
		wd->drvp->drive_flags |= DRIVE_RESET;
	errstring = readdisklabel(MAKEWDDEV(0, wd->sc_dev.dv_unit, RAW_PART),
			wdstrategy, lp);
	if (errstring) {
		/*
		 * This probably happened because the drive's default
		 * geometry doesn't match the DOS geometry.  We
		 * assume the DOS geometry is now in the label and try
		 * again.  XXX This is a kluge.
		 */
		if (wd->drvp->state > RESET)
			wd->drvp->drive_flags |= DRIVE_RESET;
		errstring = readdisklabel(MAKEWDDEV(0, wd->sc_dev.dv_unit, RAW_PART),
				wdstrategy, lp);
	}
	if (errstring) {
		printf("%s: %s\n", wd->sc_dev.dv_xname, errstring);
		return;
	}

	if (wd->drvp->state > RESET)
		wd->drvp->drive_flags |= DRIVE_RESET;
#ifdef HAS_BAD144_HANDLING
	if ((lp->d_flags & D_BADSECT) != 0)
		bad144intern(wd);
#endif
}

void
wdperror(const struct wd_softc *wd)
{
	static const char *const errstr0_3[] = {"address mark not found",
	    "track 0 not found", "aborted command", "media change requested",
	    "id not found", "media changed", "uncorrectable data error",
	    "bad block detected"};
	static const char *const errstr4_5[] = {
	    "obsolete (address mark not found)",
	    "no media/write protected", "aborted command",
	    "media change requested", "id not found", "media changed",
	    "uncorrectable data error", "interface CRC error"};
	const char *const *errstr;
	int i;
	char *sep = "";

	const char *devname = wd->sc_dev.dv_xname;
	struct ata_drive_datas *drvp = wd->drvp;
	int errno = wd->sc_wdc_bio.r_error;

	if (drvp->ata_vers >= 4)
		errstr = errstr4_5;
	else
		errstr = errstr0_3;

	printf("%s: (", devname);

	if (errno == 0)
		printf("error not notified");

	for (i = 0; i < 8; i++) {
		if (errno & (1 << i)) {
			printf("%s%s", sep, errstr[i]);
			sep = ", ";
		}
	}
	printf(")\n");
}

int
wdioctl_sc(struct wd_softc *wd, dev_t dev, u_long xfer, caddr_t addr, int flag, struct proc *p)
{
	int error;

	error = 0;
	WDCDEBUG_PRINT(("wdioctl\n"), DEBUG_FUNCS);
	if ((wd->sc_flags & WDF_LOADED) == 0) {
		return (EIO);
	}

	switch (xfer) {
#ifdef HAS_BAD144_HANDLING
	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			return EBADF;
		wd->sc_dk.dk_cpulabel->bad = *(struct dkbad *)addr;
		wd->sc_dk.dk_label->d_flags |= D_BADSECT;
		bad144intern(wd);
		return (0);
#endif
	case DIOCBSLIST:
	{
		u_int32_t count, missing, skip;
		struct dkbadsecinfo dbsi;
		struct dkbadsectors *dbs;
		size_t available;
		caddr_t laddr;

		dbsi = *(struct dkbadsecinfo *)addr;
		missing = wd->sc_bscount;
		count = 0;
		available = dbsi.dbsi_bufsize;
		skip = dbsi.dbsi_skip;
		laddr = dbsi.dbsi_buffer;

		/*
		 * We start this loop with the expectation that all of the
		 * entries will be missed and decrement this counter each
		 * time we either skip over one (already copied out) or
		 * we actually copy it back to user space.  The structs
		 * holding the bad sector information are copied directly
		 * back to user space whilst the summary is returned via
		 * the struct passed in via the ioctl.
		 */
		SLIST_FOREACH(dbs, &wd->sc_bslist, dbs_next) {
			if (skip > 0) {
				missing--;
				skip--;
				continue;
			}
			if (available < sizeof(*dbs))
				break;
			available -= sizeof(*dbs);
			copyout(dbs, laddr, sizeof(*dbs));
			laddr += sizeof(*dbs);
			missing--;
			count++;
		}
		dbsi.dbsi_left = missing;
		dbsi.dbsi_copied = count;
		*(struct dkbadsecinfo*) addr = dbsi;
		return (0);
	}

	case DIOCBSFLUSH:
		/* Clean out the bad sector list */
		while (!SLIST_EMPTY(&wd->sc_bslist)) {
			void *head = SLIST_FIRST(&wd->sc_bslist);
			SLIST_REMOVE_HEAD(&wd->sc_bslist, dbs_next);
			free(head, M_TEMP);
		}
		wd->sc_bscount = 0;
		return (0);

	case DIOCSDINFO:
	{
		struct disklabel *lp;

		if ((flag & FWRITE) == 0) {
			return (EBADF);
		}

		lp = (struct disklabel*) addr;

		if ((error = lockmgr(&wd->sc_lock, LK_EXCLUSIVE, NULL, LOCKHOLDER_PID(&wd->sc_lock.lk_lockholder))) != 0) {
			return (error);
		}
		wd->sc_flags |= WDF_LABELLING;

		error = setdisklabel(wd->sc_dk.dk_label, lp, 0);
		if (error == 0) {
			if (wd->drvp->state > RESET)
				wd->drvp->drive_flags |= DRIVE_RESET;
			if (xfer == DIOCWDINFO)
				error = writedisklabel(WDLABELDEV(dev), wdstrategy,
						wd->sc_dk.dk_label);
		}

		wd->sc_flags &= ~WDF_LABELLING;
		lockmgr(&wd->sc_lock, LK_RELEASE, NULL, LOCKHOLDER_PID(&wd->sc_lock.lk_lockholder));
		return (error);
	}

	case DIOCGDEFLABEL:
		wdgetdefaultlabel(wd, (struct disklabel *)addr);
		return (0);
#ifdef notyet
	case DIOCWFORMAT:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		{
			register struct format_op *fop;
			struct iovec aiov;
			struct uio auio;

			fop = (struct format_op*) addr;
			aiov.iov_base = fop->df_buf;
			aiov.iov_len = fop->df_count;
			auio.uio_iov = &aiov;
			auio.uio_iovcnt = 1;
			auio.uio_resid = fop->df_count;
			auio.uio_segflg = 0;
			auio.uio_offset = fop->df_startblk * wd->sc_dk.dk_label->d_secsize;
			auio.uio_procp = p;
			error = physio(wdformat, NULL, dev, B_WRITE, minphys, &auio);
			fop->df_count -= auio.uio_resid;
			fop->df_reg[0] = wdc->sc_status;
			fop->df_reg[1] = wdc->sc_error;
			return (error);
		}
#endif
	case DIOCGCACHE:
		return (wd_getcache(wd, (int *)addr));

	case DIOCSCACHE:
		return (wd_setcache(wd, *(int *)addr));

	case DIOCCACHESYNC:
		return (wd_flushcache(wd, AT_WAIT));

	case ATAIOCCOMMAND:
		/*
		 * Make sure this command is (relatively) safe first
		 */
		if ((((atareq_t *) addr)->flags & ATACMD_READ) == 0 &&
		    (flag & FWRITE) == 0)
			return (EBADF);
		{
			struct wd_ioctl *wi;
			atareq_t *atareq = (atareq_t*) addr;
			int error;

			wi = wi_get();
			wi->wi_softc = wd;
			wi->wi_atareq = *atareq;

			if (atareq->datalen
					&& (atareq->flags & (ATACMD_READ | ATACMD_WRITE))) {
				wi->wi_iov.iov_base = atareq->databuf;
				wi->wi_iov.iov_len = atareq->datalen;
				wi->wi_uio.uio_iov = &wi->wi_iov;
				wi->wi_uio.uio_iovcnt = 1;
				wi->wi_uio.uio_resid = atareq->datalen;
				wi->wi_uio.uio_offset = 0;
				wi->wi_uio.uio_segflg = UIO_USERSPACE;
				wi->wi_uio.uio_rw =
						(atareq->flags & ATACMD_READ) ? B_READ : B_WRITE;
				wi->wi_uio.uio_procp = p;
				error = physio(wdioctlstrategy, &wi->wi_bp, dev,
						(atareq->flags & ATACMD_READ) ? B_READ : B_WRITE,
						minphys, &wi->wi_uio);
			} else {
				/* No need to call physio if we don't have any
				 user data */
				wi->wi_bp.b_flags = 0;
				wi->wi_bp.b_data = 0;
				wi->wi_bp.b_bcount = 0;
				wi->wi_bp.b_dev = 0;
				wi->wi_bp.b_proc = p;
				wdioctlstrategy(&wi->wi_bp);
				error = wi->wi_bp.b_error;
			}
			*atareq = wi->wi_atareq;
			wi_free(wi);
			return (error);
		}

	default:
		return (ENOTTY);
	}

#ifdef DIAGNOSTIC
	panic("wdioctl: impossible");
#endif
}

int
wdioctl(dev_t dev, u_long xfer, caddr_t addr, int flag, struct proc *p)
{
	struct wd_softc *wd;
	int error;

	wd = wd_cd.cd_devs[WDUNIT(dev)];
	error = wdioctl_sc(wd, dev, xfer, addr, flag, p);
	if (error != 0) {
		error = ioctldisklabel(&wd->sc_dk, wdstrategy, dev, xfer, addr, flag);
	}
	return (error);
}

#ifdef B_FORMAT
int
wdformat(struct buf *bp)
{

	bp->b_flags |= B_FORMAT;
	return (wdstrategy(bp));
}
#endif

int
wdsize(dev_t dev)
{
	struct wd_softc *wd;
	int part, omask;
	int size;

	WDCDEBUG_PRINT(("wdsize\n"), DEBUG_FUNCS);

	wd = wd_cd.cd_devs[WDUNIT(dev)];
	if (wd == NULL)
		return (-1);

	part = WDPART(dev);
	omask = wd->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && wdopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	if (wd->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = wd->sc_dk.dk_label->d_partitions[part].p_size *
		    (wd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && wdclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	return (size);
}

/* #define WD_DUMP_NOT_TRUSTED if you just want to watch */
static int wddoingadump = 0;
static int wddumprecalibrated = 0;
static int wddumpmulti = 1;

/*
 * Dump core after a system crash.
 */
int
wddump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	struct wd_softc *wd;	/* disk unit to do the I/O */
	struct disklabel *lp;   /* disk's disklabel */
	int part, err;
	int nblks;	/* total number of sectors left to write */

	/* Check if recursive dump; if so, punt. */
	if (wddoingadump)
		return EFAULT;
	wddoingadump = 1;

	wd = wd_cd.cd_devs[WDUNIT(dev)];
	if (wd == NULL)
		return (ENXIO);

	part = WDPART(dev);

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = wd->sc_dk.dk_label;
	if ((size % lp->d_secsize) != 0)
		return EFAULT;
	nblks = size / lp->d_secsize;
	blkno = blkno / (lp->d_secsize / DEV_BSIZE);

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + nblks) > lp->d_partitions[part].p_size))
		return EINVAL;

	/* Offset block number to start of partition. */
	blkno += lp->d_partitions[part].p_offset;

	/* Recalibrate, if first dump transfer. */
	if (wddumprecalibrated == 0) {
		wddumpmulti = wd->sc_multi;
		wddumprecalibrated = 1;
		wd->atabus->ata_reset_channel(wd->drvp, AT_POLL | AT_RST_EMERG);
		wd->drvp->state = RESET;
	}

	while (nblks > 0) {
		wd->sc_bp = NULL;
		wd->sc_wdc_bio.blkno = blkno;
		wd->sc_wdc_bio.flags = ATA_POLL;
		if ((wd->sc_flags & WDF_LBA48) &&
		    (blkno > LBA48_THRESHOLD ||
	    	    (wd->sc_quirks & WD_QUIRK_FORCE_LBA48) != 0))
			wd->sc_wdc_bio.flags |= ATA_LBA48;
		if (wd->sc_flags & WDF_LBA)
			wd->sc_wdc_bio.flags |= ATA_LBA;
		wd->sc_wdc_bio.bcount =
			min(nblks, wddumpmulti) * lp->d_secsize;
		wd->sc_wdc_bio.databuf = va;
#ifndef WD_DUMP_NOT_TRUSTED
		switch (wd->atabus->ata_bio(wd->drvp, &wd->sc_wdc_bio)) {
		case WDC_TRY_AGAIN:
			panic("wddump: try again");
			break;
		case WDC_QUEUED:
			panic("wddump: polled command has been queued");
			break;
		case WDC_COMPLETE:
			break;
		}
		switch(wd->sc_wdc_bio.error) {
		case TIMEOUT:
			printf("wddump: device timed out");
			err = EIO;
			break;
		case ERR_DF:
			printf("wddump: drive fault");
			err = EIO;
			break;
		case ERR_DMA:
			printf("wddump: DMA error");
			err = EIO;
			break;
		case ERROR:
			printf("wddump: ");
			wdperror(wd);
			err = EIO;
			break;
		case NOERROR:
			err = 0;
			break;
		default:
			panic("wddump: unknown error type");
		}
		if (err != 0) {
			printf("\n");
			return err;
		}
#else	/* WD_DUMP_NOT_TRUSTED */
		/* Let's just talk about this first... */
		printf("wd%d: dump addr 0x%x, cylin %d, head %d, sector %d\n",
		    unit, va, cylin, head, sector);
		delay(500 * 1000);	/* half a second */
#endif

		/* update block count */
		nblks -= min(nblks, wddumpmulti);
		blkno += min(nblks, wddumpmulti);
		va += min(nblks, wddumpmulti) * lp->d_secsize;
	}

	wddoingadump = 0;
	return 0;
}

#ifdef HAS_BAD144_HANDLING
/*
 * Internalize the bad sector table.
 */
void
bad144intern(struct wd_softc *wd)
{
	struct dkbad *bt = &wd->sc_dk.dk_cpulabel->bad;
	struct disklabel *lp = wd->sc_dk.dk_label;
	int i = 0;

	WDCDEBUG_PRINT(("bad144intern\n"), DEBUG_XFERS);

	for (; i < MAXBAD; i++) {
		if (bt->bt_bad[i].bt_cyl == 0xffff)
			break;
		wd->sc_badsect[i] =
		    bt->bt_bad[i].bt_cyl * lp->d_secpercyl +
		    (bt->bt_bad[i].bt_trksec >> 8) * lp->d_nsectors +
		    (bt->bt_bad[i].bt_trksec & 0xff);
	}
	for (; i < MAXBAD+1; i++)
		wd->sc_badsect[i] = -1;
}
#endif

int
wd_get_params(struct wd_softc *wd, u_int8_t flags, struct ataparams *params)
{
	switch (wd->atabus->ata_get_params(wd->drvp, flags, params)) {
	case CMD_AGAIN:
		return 1;
	case CMD_ERR:
		/*
		 * We `know' there's a drive here; just assume it's old.
		 * This geometry is only used to read the MBR and print a
		 * (false) attach message.
		 */
		strncpy(params->atap_model, "ST506",
		    sizeof params->atap_model);
		params->atap_config = ATA_CFG_FIXED;
		params->atap_cylinders = 1024;
		params->atap_heads = 8;
		params->atap_sectors = 17;
		params->atap_multi = 1;
		params->atap_capabilities1 = params->atap_capabilities2 = 0;
		wd->drvp->ata_vers = -1; /* Mark it as pre-ATA */
		return 0;
	case CMD_OK:
		return 0;
	default:
		panic("wd_get_params: bad return code from ata_get_params");
		/* NOTREACHED */
	}
}

int
wd_getcache(struct wd_softc *wd, int *bitsp)
{
	struct ataparams params;

	if (wd_get_params(wd, AT_WAIT, &params) != 0)
		return EIO;
	if (params.atap_cmd_set1 == 0x0000 ||
	    params.atap_cmd_set1 == 0xffff ||
	    (params.atap_cmd_set1 & WDC_CMD1_CACHE) == 0) {
		*bitsp = 0;
		return 0;
	}
	*bitsp = DKCACHE_WCHANGE | DKCACHE_READ;
	if (params.atap_cmd1_en & WDC_CMD1_CACHE)
		*bitsp |= DKCACHE_WRITE;

	return 0;
}

int
wd_setcache(struct wd_softc *wd, int bits)
{
	struct ataparams params;
	struct wdc_command wdc_c;

	if (wd_get_params(wd, AT_WAIT, &params) != 0)
		return EIO;

	if (params.atap_cmd_set1 == 0x0000 ||
	    params.atap_cmd_set1 == 0xffff ||
	    (params.atap_cmd_set1 & WDC_CMD1_CACHE) == 0)
		return EOPNOTSUPP;

	if ((bits & DKCACHE_READ) == 0 ||
	    (bits & DKCACHE_SAVE) != 0)
		return EOPNOTSUPP;

	memset(&wdc_c, 0, sizeof(struct wdc_command));
	wdc_c.r_command = SET_FEATURES;
	wdc_c.r_st_bmask = 0;
	wdc_c.r_st_pmask = 0;
	wdc_c.timeout = 30000; /* 30s timeout */
	wdc_c.flags = AT_WAIT;
	if (bits & DKCACHE_WRITE)
		wdc_c.r_precomp = WDSF_WRITE_CACHE_EN;
	else
		wdc_c.r_precomp = WDSF_WRITE_CACHE_DS;
	if (wd->atabus->ata_exec_command(wd->drvp, &wdc_c) != WDC_COMPLETE) {
		printf("%s: wd_setcache command not complete\n",
		    wd->sc_dev.dv_xname);
		return EIO;
	}
	if (wdc_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		printf("%s: wd_setcache command error 0x%x\n",
		    wd->sc_dev.dv_xname, wdc_c.flags);
		return EIO;
	}
	if (wdc_c.flags & ERR_NODEV)
		return ENODEV;
	return 0;
}

int
wd_flushcache(struct wd_softc *wd, int flags)
{
	struct wdc_command wdc_c;

	/*
	 * WDCC_FLUSHCACHE is here since ATA-4, but some drives report
	 * only ATA-2 and still support it.
	 */
	if (wd->drvp->ata_vers < 4 &&
	    ((wd->sc_params.atap_cmd_set2 & WDC_CMD2_FC) == 0 ||
	    wd->sc_params.atap_cmd_set2 == 0xffff))
		return ENODEV;
	memset(&wdc_c, 0, sizeof(struct wdc_command));
	if ((wd->sc_params.atap_cmd2_en & ATA_CMD2_LBA48) != 0 &&
	    (wd->sc_params.atap_cmd2_en & ATA_CMD2_FCE) != 0)
		wdc_c.r_command = WDCC_FLUSHCACHE_EXT;
	else
		wdc_c.r_command = WDCC_FLUSHCACHE;
	wdc_c.r_st_bmask = WDCS_DRDY;
	wdc_c.r_st_pmask = WDCS_DRDY;
	wdc_c.flags = flags;
	wdc_c.timeout = 30000; /* 30s timeout */
	if (wd->atabus->ata_exec_command(wd->drvp, &wdc_c) != WDC_COMPLETE) {
		printf("%s: flush cache command didn't complete\n",
		    wd->sc_dev.dv_xname);
		return EIO;
	}
	if (wdc_c.flags & ERR_NODEV)
		return ENODEV;
	if (wdc_c.flags & AT_TIMEOU) {
		printf("%s: flush cache command timeout\n",
		    wd->sc_dev.dv_xname);
		return EIO;
	}
	if (wdc_c.flags & AT_ERROR) {
		if (wdc_c.r_error == WDCE_ABRT) /* command not supported */
			return ENODEV;
		printf("%s: flush cache command: error 0x%x\n",
		    wd->sc_dev.dv_xname, wdc_c.r_error);
		return EIO;
	}
	if (wdc_c.flags & AT_DF) {
		printf("%s: flush cache command: drive fault\n",
		    wd->sc_dev.dv_xname);
		return EIO;
	}
	return 0;
}

void
wd_shutdown(void *arg)
{
	struct wd_softc *wd = arg;
	wd_flushcache(wd, AT_POLL);
}

/*
 * Allocate space for a ioctl queue structure.  Mostly taken from
 * scsipi_ioctl.c
 */
struct wd_ioctl *
wi_get(void)
{
	struct wd_ioctl *wi;
	int s;

	wi = malloc(sizeof(struct wd_ioctl), M_TEMP, M_WAITOK|M_ZERO);
	simple_lock_init(&wi->wi_bp.b_lnterlock, "wd_buf_slock");
	s = splbio();
	LIST_INSERT_HEAD(&wi_head, wi, wi_list);
	splx(s);
	return (wi);
}

/*
 * Free an ioctl structure and remove it from our list
 */

void
wi_free(struct wd_ioctl *wi)
{
	int s;

	s = splbio();
	LIST_REMOVE(wi, wi_list);
	splx(s);
	free(wi, M_TEMP);
}

/*
 * Find a wd_ioctl structure based on the struct buf.
 */

struct wd_ioctl *
wi_find(struct buf *bp)
{
	struct wd_ioctl *wi;
	int s;

	s = splbio();
	for (wi = LIST_FIRST(&wi_head); wi != 0; wi = LIST_NEXT(wi, wi_list))
		if (bp == &wi->wi_bp)
			break;
	splx(s);
	return (wi);
}

/*
 * Ioctl pseudo strategy routine
 *
 * This is mostly stolen from scsipi_ioctl.c:scsistrategy().  What
 * happens here is:
 *
 * - wdioctl() queues a wd_ioctl structure.
 *
 * - wdioctl() calls physio/wdioctlstrategy based on whether or not
 *   user space I/O is required.  If physio() is called, physio() eventually
 *   calls wdioctlstrategy().
 *
 * - In either case, wdioctlstrategy() calls wd->atabus->ata_exec_command()
 *   to perform the actual command
 *
 * The reason for the use of the pseudo strategy routine is because
 * when doing I/O to/from user space, physio _really_ wants to be in
 * the loop.  We could put the entire buffer into the ioctl request
 * structure, but that won't scale if we want to do things like download
 * microcode.
 */

void
wdioctlstrategy(struct buf *bp)
{
	struct wd_ioctl *wi;
	struct wdc_command wdc_c;
	int error = 0;

	wi = wi_find(bp);
	if (wi == NULL) {
		printf("user_strat: No ioctl\n");
		error = EINVAL;
		goto bad;
	}

	memset(&wdc_c, 0, sizeof(wdc_c));

	/*
	 * Abort if physio broke up the transfer
	 */

	if (bp->b_bcount != wi->wi_atareq.datalen) {
		printf("physio split wd ioctl request... cannot proceed\n");
		error = EIO;
		goto bad;
	}

	/*
	 * Abort if we didn't get a buffer size that was a multiple of
	 * our sector size (or was larger than NBBY)
	 */

	if ((bp->b_bcount % wi->wi_softc->sc_dk.dk_label->d_secsize) != 0 ||
	    (bp->b_bcount / wi->wi_softc->sc_dk.dk_label->d_secsize) >=
	     (1 << NBBY)) {
		error = EINVAL;
		goto bad;
	}

	/*
	 * Make sure a timeout was supplied in the ioctl request
	 */

	if (wi->wi_atareq.timeout == 0) {
		error = EINVAL;
		goto bad;
	}

	if (wi->wi_atareq.flags & ATACMD_READ)
		wdc_c.flags |= AT_READ;
	else if (wi->wi_atareq.flags & ATACMD_WRITE)
		wdc_c.flags |= AT_WRITE;

	if (wi->wi_atareq.flags & ATACMD_READREG)
		wdc_c.flags |= AT_READREG;

	wdc_c.flags |= AT_WAIT;

	wdc_c.timeout = wi->wi_atareq.timeout;
	wdc_c.r_command = wi->wi_atareq.command;
	wdc_c.r_head = wi->wi_atareq.head & 0x0f;
	wdc_c.r_cyl = wi->wi_atareq.cylinder;
	wdc_c.r_sector = wi->wi_atareq.sec_num;
	wdc_c.r_count = wi->wi_atareq.sec_count;
	wdc_c.r_precomp = wi->wi_atareq.features;
	wdc_c.r_st_bmask = WDCS_DRDY;
	wdc_c.r_st_pmask = WDCS_DRDY;
	wdc_c.data = wi->wi_bp.b_data;
	wdc_c.bcount = wi->wi_bp.b_bcount;

	if (wi->wi_softc->atabus->ata_exec_command(wi->wi_softc->drvp, &wdc_c)
	    != WDC_COMPLETE) {
		wi->wi_atareq.retsts = ATACMD_ERROR;
		goto bad;
	}

	if (wdc_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		if (wdc_c.flags & AT_ERROR) {
			wi->wi_atareq.retsts = ATACMD_ERROR;
			wi->wi_atareq.error = wdc_c.r_error;
		} else if (wdc_c.flags & AT_DF)
			wi->wi_atareq.retsts = ATACMD_DF;
		else
			wi->wi_atareq.retsts = ATACMD_TIMEOUT;
	} else {
		wi->wi_atareq.retsts = ATACMD_OK;
		if (wi->wi_atareq.flags & ATACMD_READREG) {
			wi->wi_atareq.head = wdc_c.r_head ;
			wi->wi_atareq.cylinder = wdc_c.r_cyl;
			wi->wi_atareq.sec_num = wdc_c.r_sector;
			wi->wi_atareq.sec_count = wdc_c.r_count;
			wi->wi_atareq.features = wdc_c.r_precomp;
			wi->wi_atareq.error = wdc_c.r_error;
		}
	}

	bp->b_error = 0;
	biodone(bp);
	return;
bad:
	bp->b_flags |= B_ERROR;
	bp->b_error = error;
	biodone(bp);
}
