/*	$NetBSD: sd.c,v 1.216.2.3 2004/09/11 12:55:11 he Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*
 * Originally written by Julian Elischer (julian@dialix.oz.au)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@dialix.oz.au) Sept 1992
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sd.c,v 1.216.2.3 2004/09/11 12:55:11 he Exp $");

#include "opt_scsi.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/scsiio.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/vnode.h>
#include <sys/power.h>

#include <dev/disk/scsi/scsipi_all.h>
#include <dev/disk/scsi/scsi_all.h>
#include <dev/disk/scsi/scsipi_disk.h>
#include <dev/disk/scsi/scsi_disk.h>
#include <dev/disk/scsi/scsiconf.h>
#include <dev/disk/scsi/scsipi_base.h>
#include <dev/disk/scsi/sdvar.h>

#define	SDUNIT(dev)					dkunit(dev)
#define	SDPART(dev)					dkpart(dev)
#define	SDMINOR(unit, part)			dkminor(unit, part)
#define	MAKESDDEV(maj, unit, part)	dkmakedev(maj, unit, part)

#define	SDLABELDEV(dev)				(MAKESDDEV(major(dev), SDUNIT(dev), RAW_PART))

int		sdlock(struct sd_softc *);
void	sdunlock(struct sd_softc *);
void	sdminphys(struct buf *);
void	sdgetdefaultlabel(struct sd_softc *, struct disklabel *);
void	sdgetdisklabel(struct sd_softc *);
void	sdstart(struct scsipi_periph *);
void	sdrestart(void *);
void	sddone(struct scsipi_xfer *);
void	sd_shutdown(void *);
int		sd_reassign_blocks(struct sd_softc *, u_long);
int		sd_interpret_sense(struct scsipi_xfer *);

int	sd_mode_sense(struct sd_softc *, u_int8_t, void *, size_t, int, int, int *);
int	sd_mode_select(struct sd_softc *, u_int8_t, void *, size_t, int, int);
int	sd_get_simplifiedparms(struct sd_softc *, struct disk_parms *, int);
int	sd_get_capacity(struct sd_softc *, struct disk_parms *, int);
int	sd_get_parms(struct sd_softc *, struct disk_parms *, int);
int	sd_get_parms_page4(struct sd_softc *, struct disk_parms *, int);
int	sd_get_parms_page5(struct sd_softc *, struct disk_parms *, int);

int	sd_flush (struct sd_softc *, int);
int	sd_getcache (struct sd_softc *, int *);
int	sd_setcache (struct sd_softc *, int);

int	sdmatch (struct device *, struct cfdata *, void *);
void sdattach(struct device *, struct device *, void *);
int	sdactivate(struct device *, enum devact);
int	sddetach(struct device *, int);

CFOPS_DECL(sd, sdmatch, sdattach, sddetach, sdactivate);
CFDRIVER_DECL(NULL, sd, DV_DISK);
CFATTACH_DECL(sd, &sd_cd, &sd_cops, sizeof(struct sd_softc));

extern struct cfdriver sd_cd;

const struct scsipi_inquiry_pattern sd_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "",         "",                 ""},
	{T_OPTICAL, T_FIXED,
	 "",         "",                 ""},
	{T_OPTICAL, T_REMOV,
	 "",         "",                 ""},
	{T_SIMPLE_DIRECT, T_FIXED,
	 "",         "",                 ""},
	{T_SIMPLE_DIRECT, T_REMOV,
	 "",         "",                 ""},
};

dev_type_open(sdopen);
dev_type_close(sdclose);
dev_type_read(sdread);
dev_type_write(sdwrite);
dev_type_ioctl(sdioctl);
dev_type_strategy(sdstrategy);
dev_type_dump(sddump);
dev_type_size(sdsize);

const struct bdevsw sd_bdevsw = {
		.d_open = sdopen,
		.d_close = sdclose,
		.d_strategy = sdstrategy,
		.d_ioctl = sdioctl,
		.d_dump = sddump,
		.d_psize = sdsize,
		.d_discard = nodiscard,
		.d_type = D_DISK
};

const struct cdevsw sd_cdevsw = {
		.d_open = sdopen,
		.d_close = sdclose,
		.d_read = sdread,
		.d_write = sdwrite,
		.d_ioctl = sdioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = nommap,
		.d_discard = nodiscard,
		.d_type = D_DISK
};

struct dkdriver sddkdriver = { sdstrategy };

const struct scsipi_periphsw sd_switch = {
	sd_interpret_sense,	/* check our error handler first */
	sdstart,		/* have a queue, served by this */
	NULL,			/* have no async handler */
	sddone,			/* deal with stats at interrupt time */
};

struct sd_mode_sense_data {
	/*
	 * XXX
	 * We are not going to parse this as-is -- it just has to be large
	 * enough.
	 */
	union {
		struct scsipi_mode_header small;
		struct scsipi_mode_header_big big;
	} header;
	struct scsi_blk_desc blk_desc;
	union scsi_disk_pages pages;
};

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
int
sdmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)sd_patterns, sizeof(sd_patterns) / sizeof(sd_patterns[0]),
	    sizeof(sd_patterns[0]), &priority);

	return (priority);
}

/*
 * Attach routine common to atapi & scsi.
 */
void
sdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sd_softc *sd = (void *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;
	int error, result;
	struct disk_parms *dp = &sd->params;
	char pbuf[9];

	SC_DEBUG(periph, SCSIPI_DB2, ("sdattach: "));

	sd->type = (sa->sa_inqbuf.type & SID_TYPE);
	if (sd->type == T_SIMPLE_DIRECT)
		periph->periph_quirks |= PQUIRK_ONLYBIG | PQUIRK_NOBIGMODESENSE;

	if (scsipi_periph_bustype(sa->sa_periph) == SCSIPI_BUSTYPE_SCSI &&
	    periph->periph_version == 0)
		sd->flags |= SDF_ANCIENT;

	bufq_alloc(&sd->buf_queue,
	    BUFQ_DISK_DEFAULT_STRAT()|BUFQ_SORT_RAWBLOCK);

	callout_init(&sd->sc_callout);

	/*
	 * Store information needed to contact our base driver
	 */
	sd->sc_periph = periph;

	periph->periph_dev = &sd->sc_dev;
	periph->periph_switch = &sd_switch;

        /*
         * Increase our openings to the maximum-per-periph
         * supported by the adapter.  This will either be
         * clamped down or grown by the adapter if necessary.
         */
	periph->periph_openings =
	    SCSIPI_CHAN_MAX_PERIPH(periph->periph_channel);
	periph->periph_flags |= PERIPH_GROW_OPENINGS;

	/*
	 * Initialize and attach the disk structure.
	 */
	sd->sc_dk.dk_driver = &sddkdriver;
	sd->sc_dk.dk_name = sd->sc_dev.dv_xname;
	disk_attach(&sd->sc_dk, &sd_bdevsw, &sd_cdevsw);

	/*
	 * Use the subdriver to request information regarding the drive.
	 */
	printf("\n");
	printf("\n");

	error = scsipi_test_unit_ready(periph,
	    XS_CTL_DISCOVERY | XS_CTL_IGNORE_ILLEGAL_REQUEST |
	    XS_CTL_IGNORE_MEDIA_CHANGE | XS_CTL_SILENT_NODEV);

	if (error)
		result = SDGP_RESULT_OFFLINE;
	else
		result = sd_get_parms(sd, &sd->params, XS_CTL_DISCOVERY);
	printf("%s: ", sd->sc_dev.dv_xname);
	switch (result) {
	case SDGP_RESULT_OK:
		//format_bytes(pbuf, sizeof(pbuf), (u_int64_t)dp->disksize * dp->blksize);
		printf("%s, %ld cyl, %ld head, %ld sec, %ld bytes/sect x %llu sectors",
				pbuf, dp->cyls, dp->heads, dp->sectors, dp->blksize,
				(unsigned long long) dp->disksize);
		break;

	case SDGP_RESULT_OFFLINE:
		printf("drive offline");
		break;

	case SDGP_RESULT_UNFORMATTED:
		printf("unformatted media");
		break;

#ifdef DIAGNOSTIC
	default:
		panic("sdattach: unknown result from get_parms");
		break;
#endif
	}
	printf("\n");

	/*
	 * Establish a shutdown hook so that we can ensure that
	 * our data has actually made it onto the platter at
	 * shutdown time.  Note that this relies on the fact
	 * that the shutdown hook code puts us at the head of
	 * the list (thus guaranteeing that our hook runs before
	 * our ancestors').
	 */
	if ((sd->sc_sdhook =
	    shutdownhook_establish(sd_shutdown, sd)) == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sd->sc_dev.dv_xname);
}

int
sdactivate(self, act)
	struct device *self;
	enum devact act;
{
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		rv = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		/*
		 * Nothing to do; we key off the device's DVF_ACTIVE.
		 */
		break;
	}
	return (rv);
}

int
sddetach(self, flags)
	struct device *self;
	int flags;
{
	struct sd_softc *sd = (struct sd_softc *) self;
	struct buf *bp;
	int s, bmaj, cmaj, i, mn;

	/* locate the major number */
	bmaj = bdevsw_lookup_major(&sd_bdevsw);
	cmaj = cdevsw_lookup_major(&sd_cdevsw);

	/* kill any pending restart */
	callout_stop(&sd->sc_callout);

	s = splbio();

	/* Kill off any queued buffers. */
	while ((bp = BUFQ_GET(&sd->buf_queue)) != NULL) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
	}

	bufq_free(&sd->buf_queue);

	/* Kill off any pending commands. */
	scsipi_kill_pending(sd->sc_periph);

	splx(s);

	/* Nuke the vnodes for any open instances */
	for (i = 0; i < MAXPARTITIONS; i++) {
		mn = SDMINOR(self->dv_unit, i);
		vdevgone(bmaj, mn, mn, VBLK);
		vdevgone(cmaj, mn, mn, VCHR);
	}

	/* Detach from the disk list. */
	disk_detach(&sd->sc_dk);

	/* Get rid of the shutdown hook. */
	shutdownhook_disestablish(sd->sc_sdhook);

	return (0);
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
int
sdlock(sd)
	struct sd_softc *sd;
{
	int error;

	while ((sd->flags & SDF_LOCKED) != 0) {
		sd->flags |= SDF_WANTED;
		if ((error = tsleep(sd, PRIBIO | PCATCH, "sdlck", 0)) != 0)
			return (error);
	}
	sd->flags |= SDF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
void
sdunlock(sd)
	struct sd_softc *sd;
{

	sd->flags &= ~SDF_LOCKED;
	if ((sd->flags & SDF_WANTED) != 0) {
		sd->flags &= ~SDF_WANTED;
		wakeup(sd);
	}
}

/*
 * open the device. Make sure the partition info is a up-to-date as can be.
 */
int
sdopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct sd_softc *sd;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;
	int unit, part;
	int error;

	unit = SDUNIT(dev);
	if (unit >= sd_cd.cd_ndevs)
		return (ENXIO);
	sd = sd_cd.cd_devs[unit];
	if (sd == NULL)
		return (ENXIO);

	if ((sd->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	periph = sd->sc_periph;
	adapt = periph->periph_channel->chan_adapter;
	part = SDPART(dev);

	SC_DEBUG(periph, SCSIPI_DB1,
	    ("sdopen: dev=0x%x (unit %d (of %d), partition %d)\n", dev, unit,
	    sd_cd.cd_ndevs, part));

	/*
	 * If this is the first open of this device, add a reference
	 * to the adapter.
	 */
	if (sd->sc_dk.dk_openmask == 0 &&
	    (error = scsipi_adapter_addref(adapt)) != 0)
		return (error);

	if ((error = sdlock(sd)) != 0)
		goto bad4;

	if ((periph->periph_flags & PERIPH_OPEN) != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens of non-raw partition
		 */
		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 &&
		    (part != RAW_PART || fmt != S_IFCHR)) {
			error = EIO;
			goto bad3;
		}
	} else {
		int silent;

		if (part == RAW_PART && fmt == S_IFCHR)
			silent = XS_CTL_SILENT;
		else
			silent = 0;

		/* Check that it is still responding and ok. */
		error = scsipi_test_unit_ready(periph,
		    XS_CTL_IGNORE_ILLEGAL_REQUEST | XS_CTL_IGNORE_MEDIA_CHANGE |
		    silent);

		/*
		 * Start the pack spinning if necessary. Always allow the
		 * raw parition to be opened, for raw IOCTLs. Data transfers
		 * will check for SDEV_MEDIA_LOADED.
		 */
		if (error == EIO) {
			int error2;

			error2 = scsipi_start(periph, SSS_START, silent);
			switch (error2) {
			case 0:
				error = 0;
				break;
			case EIO:
			case EINVAL:
				break;
			default:
				error = error2;
				break;
			}
		}
		if (error) {
			if (silent)
				goto out;
			goto bad3;
		}

		periph->periph_flags |= PERIPH_OPEN;

		if (periph->periph_flags & PERIPH_REMOVABLE) {
			/* Lock the pack in. */
			error = scsipi_prevent(periph, PR_PREVENT,
			    XS_CTL_IGNORE_ILLEGAL_REQUEST |
			    XS_CTL_IGNORE_MEDIA_CHANGE);
			if (error)
				goto bad;
		}

		if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
			int param_error;
			periph->periph_flags |= PERIPH_MEDIA_LOADED;

			/*
			 * Load the physical device parameters.
			 *
			 * Note that if media is present but unformatted,
			 * we allow the open (so that it can be formatted!).
			 * The drive should refuse real I/O, if the media is
			 * unformatted.
			 */
			if ((param_error = sd_get_parms(sd, &sd->params, 0))
			     == SDGP_RESULT_OFFLINE) {
				error = ENXIO;
				goto bad2;
			}
			SC_DEBUG(periph, SCSIPI_DB3, ("Params loaded "));

			/* Load the partition info if not already loaded. */
			if (param_error == 0) {
				sdgetdisklabel(sd);
				SC_DEBUG(periph, SCSIPI_DB3,
				     ("Disklabel loaded "));
			}
		}
	}

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= sd->sc_dk.dk_label->d_npartitions ||
	     sd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

out:	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	sd->sc_dk.dk_openmask =
	    sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	SC_DEBUG(periph, SCSIPI_DB3, ("open complete\n"));
	sdunlock(sd);
	return (0);

bad2:
	periph->periph_flags &= ~PERIPH_MEDIA_LOADED;

bad:
	if (sd->sc_dk.dk_openmask == 0) {
		if (periph->periph_flags & PERIPH_REMOVABLE)
			scsipi_prevent(periph, PR_ALLOW,
			    XS_CTL_IGNORE_ILLEGAL_REQUEST |
			    XS_CTL_IGNORE_MEDIA_CHANGE);
		periph->periph_flags &= ~PERIPH_OPEN;
	}

bad3:
	sdunlock(sd);
bad4:
	if (sd->sc_dk.dk_openmask == 0)
		scsipi_adapter_delref(adapt);
	return (error);
}

/*
 * close the device.. only called if we are the LAST occurence of an open
 * device.  Convenient now but usually a pain.
 */
int
sdclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(dev)];
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	int part = SDPART(dev);
	int error;

	if ((error = sdlock(sd)) != 0)
		return (error);

	switch (fmt) {
	case S_IFCHR:
		sd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		sd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sd->sc_dk.dk_openmask =
	    sd->sc_dk.dk_copenmask | sd->sc_dk.dk_bopenmask;

	if (sd->sc_dk.dk_openmask == 0) {
		/*
		 * If the disk cache needs flushing, and the disk supports
		 * it, do it now.
		 */
		if ((sd->flags & SDF_DIRTY) != 0) {
			if (sd_flush(sd, 0)) {
				printf("%s: cache synchronization failed\n",
				    sd->sc_dev.dv_xname);
				sd->flags &= ~SDF_FLUSHING;
			} else
				sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
		}

		if (! (periph->periph_flags & PERIPH_KEEP_LABEL))
			periph->periph_flags &= ~PERIPH_MEDIA_LOADED;

		scsipi_wait_drain(periph);

		if (periph->periph_flags & PERIPH_REMOVABLE)
			scsipi_prevent(periph, PR_ALLOW,
			    XS_CTL_IGNORE_ILLEGAL_REQUEST |
			    XS_CTL_IGNORE_NOT_READY);
		periph->periph_flags &= ~PERIPH_OPEN;

		scsipi_wait_drain(periph);

		scsipi_adapter_delref(adapt);
	}

	sdunlock(sd);
	return (0);
}

/*
 * Actually translate the requested transfer into one the physical driver
 * can understand.  The transfer is described by a buf and will include
 * only one physical transfer.
 */
void
sdstrategy(bp)
	struct buf *bp;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(bp->b_dev)];
	struct scsipi_periph *periph = sd->sc_periph;
	struct disklabel *lp;
	daddr_t blkno;
	int s;
	bool_t sector_aligned;

	SC_DEBUG(sd->sc_periph, SCSIPI_DB2, ("sdstrategy "));
	SC_DEBUG(sd->sc_periph, SCSIPI_DB1,
	    ("%ld bytes @ blk %" PRId64 "\n", bp->b_bcount, bp->b_blkno));
	/*
	 * If the device has been made invalid, error out
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0 ||
	    (sd->sc_dev.dv_flags & DVF_ACTIVE) == 0) {
		if (periph->periph_flags & PERIPH_OPEN)
			bp->b_error = EIO;
		else
			bp->b_error = ENODEV;
		goto bad;
	}

	lp = sd->sc_dk.dk_label;

	/*
	 * The transfer must be a whole number of blocks, offset must not be
	 * negative.
	 */
	if (lp->d_secsize == DEV_BSIZE) {
		sector_aligned = (bp->b_bcount & (DEV_BSIZE - 1)) == 0;
	} else {
		sector_aligned = (bp->b_bcount % lp->d_secsize) == 0;
	}
	if (!sector_aligned || bp->b_blkno < 0) {
		bp->b_error = EINVAL;
		goto bad;
	}
	/*
	 * If it's a null transfer, return immediatly
	 */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (SDPART(bp->b_dev) == RAW_PART) {
		if (bounds_check_with_mediasize(bp, DEV_BSIZE,
		    sd->params.disksize512) <= 0)
			goto done;
	} else {
		if (bounds_check_with_label(&sd->sc_dk, bp,
		    (sd->flags & (SDF_WLABEL|SDF_LABELLING)) != 0) <= 0)
			goto done;
	}

	/*
	 * Now convert the block number to absolute and put it in
	 * terms of the device's logical block size.
	 */
	if (lp->d_secsize == DEV_BSIZE)
		blkno = bp->b_blkno;
	else if (lp->d_secsize > DEV_BSIZE)
		blkno = bp->b_blkno / (lp->d_secsize / DEV_BSIZE);
	else
		blkno = bp->b_blkno * (DEV_BSIZE / lp->d_secsize);

	if (SDPART(bp->b_dev) != RAW_PART)
		blkno += lp->d_partitions[SDPART(bp->b_dev)].p_offset;

	bp->b_rawblkno = blkno;

	s = splbio();

	/*
	 * Place it in the queue of disk activities for this disk.
	 *
	 * XXX Only do disksort() if the current operating mode does not
	 * XXX include tagged queueing.
	 */
	BUFQ_PUT(&sd->buf_queue, bp);

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion
	 */
	sdstart(sd->sc_periph);

	splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * sdstart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It dequeues the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call scsipi_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (sdstrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
 *
 * must be called at the correct (highish) spl level
 * sdstart() is called at splbio from sdstrategy, sdrestart and scsipi_done
 */
void
sdstart(periph)
	struct scsipi_periph *periph;
{
	struct sd_softc *sd = (void *)periph->periph_dev;
	struct disklabel *lp = sd->sc_dk.dk_label;
	struct buf *bp = 0;
	struct scsipi_rw_big cmd_big;
	struct scsi_rw cmd_small;
	struct scsipi_generic *cmdp;
	struct scsipi_xfer *xs;
	int nblks, cmdlen, error, flags;

	SC_DEBUG(periph, SCSIPI_DB2, ("sdstart "));
	/*
	 * Check if the device has room for another command
	 */
	while (periph->periph_active < periph->periph_openings) {
		/*
		 * there is excess capacity, but a special waits
		 * It'll need the adapter as soon as we clear out of the
		 * way and let it run (user level wait).
		 */
		if (periph->periph_flags & PERIPH_WAITING) {
			periph->periph_flags &= ~PERIPH_WAITING;
			wakeup((caddr_t)periph);
			return;
		}

		/*
		 * If the device has become invalid, abort all the
		 * reads and writes until all files have been closed and
		 * re-opened
		 */
		if (__predict_false(
		    (periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)) {
			if ((bp = BUFQ_GET(&sd->buf_queue)) != NULL) {
				bp->b_error = EIO;
				bp->b_flags |= B_ERROR;
				bp->b_resid = bp->b_bcount;
				biodone(bp);
				continue;
			} else {
				return;
			}
		}

		/*
		 * See if there is a buf with work for us to do..
		 */
		if ((bp = BUFQ_PEEK(&sd->buf_queue)) == NULL)
			return;

		/*
		 * We have a buf, now we should make a command.
		 */

		if (lp->d_secsize == DEV_BSIZE)
			nblks = bp->b_bcount >> DEV_BSHIFT;
		else
			nblks = howmany(bp->b_bcount, lp->d_secsize);

		/*
		 *  Fill out the scsi command.  If the transfer will
		 *  fit in a "small" cdb, use it.
		 */
		if (((bp->b_rawblkno & 0x1fffff) == bp->b_rawblkno) &&
		    ((nblks & 0xff) == nblks) &&
		    !(periph->periph_quirks & PQUIRK_ONLYBIG)) {
			/*
			 * We can fit in a small cdb.
			 */
			memset(&cmd_small, 0, sizeof(cmd_small));
			cmd_small.opcode = (bp->b_flags & B_READ) ?
			    SCSI_READ_COMMAND : SCSI_WRITE_COMMAND;
			_lto3b(bp->b_rawblkno, cmd_small.addr);
			cmd_small.length = nblks & 0xff;
			cmdlen = sizeof(cmd_small);
			cmdp = (struct scsipi_generic *)&cmd_small;
		} else {
			/*
			 * Need a large cdb.
			 */
			memset(&cmd_big, 0, sizeof(cmd_big));
			cmd_big.opcode = (bp->b_flags & B_READ) ?
			    READ_BIG : WRITE_BIG;
			_lto4b(bp->b_rawblkno, cmd_big.addr);
			_lto2b(nblks, cmd_big.length);
			cmdlen = sizeof(cmd_big);
			cmdp = (struct scsipi_generic *)&cmd_big;
		}

		/* Instrumentation. */
		disk_busy(&sd->sc_dk);

		/*
		 * Mark the disk dirty so that the cache will be
		 * flushed on close.
		 */
		if ((bp->b_flags & B_READ) == 0)
			sd->flags |= SDF_DIRTY;

		/*
		 * Figure out what flags to use.
		 */
		flags = XS_CTL_NOSLEEP|XS_CTL_ASYNC|XS_CTL_SIMPLE_TAG;
		if (bp->b_flags & B_READ)
			flags |= XS_CTL_DATA_IN;
		else
			flags |= XS_CTL_DATA_OUT;

		/*
		 * Call the routine that chats with the adapter.
		 * Note: we cannot sleep as we may be an interrupt
		 */
		xs = scsipi_make_xs(periph, cmdp, cmdlen,
		    (u_char *)bp->b_data, bp->b_bcount,
		    SDRETRIES, SD_IO_TIMEOUT, bp, flags);
		if (__predict_false(xs == NULL)) {
			/*
			 * out of memory. Keep this buffer in the queue, and
			 * retry later.
			 */
			printf("sdstart(): try again\n");
			callout_reset(&sd->sc_callout, hz / 2, sdrestart,
			    periph);
			return;
		}
		/*
		 * need to dequeue the buffer before queuing the command,
		 * because cdstart may be called recursively from the
		 * HBA driver
		 */
#ifdef DIAGNOSTIC
		if (BUFQ_GET(&sd->buf_queue) != bp)
			panic("sdstart(): dequeued wrong buf");
#else
		BUFQ_GET(&sd->buf_queue);
#endif
		error = scsipi_command(periph, xs, cmdp, cmdlen,
		    (u_char *)bp->b_data, bp->b_bcount,
		    SDRETRIES, SD_IO_TIMEOUT, bp, flags);
		/* with a scsipi_xfer preallocated, scsipi_command can't fail */
		KASSERT(error == 0);
	}
}

void
sdrestart(void *v)
{
	int s = splbio();
	sdstart((struct scsipi_periph *)v);
	splx(s);
}

void
sddone(xs)
	struct scsipi_xfer *xs;
{
	struct sd_softc *sd = (void *)xs->xs_periph->periph_dev;

	if (sd->flags & SDF_FLUSHING) {
		/* Flush completed, no longer dirty. */
		sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
	}

	if (xs->bp != NULL) {
		disk_unbusy(&sd->sc_dk, xs->bp->b_bcount - xs->bp->b_resid);
	}
}

void
sdminphys(bp)
	struct buf *bp;
{
	struct sd_softc *sd = sd_cd.cd_devs[SDUNIT(bp->b_dev)];
	long max;

	/*
	 * If the device is ancient, we want to make sure that
	 * the transfer fits into a 6-byte cdb.
	 *
	 * XXX Note that the SCSI-I spec says that 256-block transfers
	 * are allowed in a 6-byte read/write, and are specified
	 * by settng the "length" to 0.  However, we're conservative
	 * here, allowing only 255-block transfers in case an
	 * ancient device gets confused by length == 0.  A length of 0
	 * in a 10-byte read/write actually means 0 blocks.
	 */
	if ((sd->flags & SDF_ANCIENT) &&
	    ((sd->sc_periph->periph_flags &
	    (PERIPH_REMOVABLE | PERIPH_MEDIA_LOADED)) != PERIPH_REMOVABLE)) {
		max = sd->sc_dk.dk_label->d_secsize * 0xff;

		if (bp->b_bcount > max)
			bp->b_bcount = max;
	}

	scsipi_adapter_minphys(sd->sc_periph->periph_channel, bp);
}

int
sdread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(sdstrategy, NULL, dev, B_READ, sdminphys, uio));
}

int
sdwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{

	return (physio(sdstrategy, NULL, dev, B_WRITE, sdminphys, uio));
}

/* internal sdioctl */
int
sdioctl_sc(sd, dev, cmd, addr, flag, p)
	struct sd_softc *sd;
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct scsipi_periph *periph = sd->sc_periph;
	int part = SDPART(dev);
	int error = 0;

	SC_DEBUG(sd->sc_periph, SCSIPI_DB2, ("sdioctl 0x%lx ", cmd));

	/*
	 * If the device is not valid, some IOCTLs can still be
	 * handled on the raw partition. Check this here.
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0) {
		switch (cmd) {
		case DIOCKLABEL:
		case DIOCWLABEL:
		case DIOCLOCK:
		case DIOCEJECT:
		case ODIOCEJECT:
		case DIOCGCACHE:
		case DIOCSCACHE:
		case SCIOCIDENTIFY:
		case OSCIOCIDENTIFY:
		case SCIOCCOMMAND:
		case SCIOCDEBUG:
			if (part == RAW_PART) {
				break;
			}
			/* FALLTHROUGH */
		default:
			if ((periph->periph_flags & PERIPH_OPEN) == 0)
				return (ENODEV);
			else
				return (EIO);
		}
	}

	switch (cmd) {
	case DIOCSDINFO:
	{
		struct disklabel *lp;

		if ((flag & FWRITE) == 0) {
			return (EBADF);
		}

		lp = (struct disklabel *)addr;

		if ((error = sdlock(sd)) != 0) {
			return (error);
		}
		sd->flags |= SDF_LABELLING;

		error = setdisklabel(sd->sc_dk.dk_label, lp, /*sd->sc_dk.dk_openmask : */
				0);
		if (error == 0) {
			if (cmd == DIOCWDINFO) {
			error = writedisklabel(SDLABELDEV(dev),
					sdstrategy, sd->sc_dk.dk_label);
			}
		}

		sd->flags &= ~SDF_LABELLING;
		sdunlock(sd);

		return (error);
	}

	case DIOCKLABEL:
		if (*(int *)addr)
			periph->periph_flags |= PERIPH_KEEP_LABEL;
		else
			periph->periph_flags &= ~PERIPH_KEEP_LABEL;
		return (0);

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)addr)
			sd->flags |= SDF_WLABEL;
		else
			sd->flags &= ~SDF_WLABEL;
		return (0);

	case DIOCLOCK:
		return (scsipi_prevent(periph,
		    (*(int *)addr) ? PR_PREVENT : PR_ALLOW, 0));

	case DIOCEJECT:
		if ((periph->periph_flags & PERIPH_REMOVABLE) == 0)
			return (ENOTTY);
		if (*(int *)addr == 0) {
			/*
			 * Don't force eject: check that we are the only
			 * partition open. If so, unlock it.
			 */
			if ((sd->sc_dk.dk_openmask & ~(1 << part)) == 0 &&
			    sd->sc_dk.dk_bopenmask + sd->sc_dk.dk_copenmask ==
			    sd->sc_dk.dk_openmask) {
				error = scsipi_prevent(periph, PR_ALLOW,
				    XS_CTL_IGNORE_NOT_READY);
				if (error)
					return (error);
			} else {
				return (EBUSY);
			}
		}
		/* FALLTHROUGH */
	case ODIOCEJECT:
		return ((periph->periph_flags & PERIPH_REMOVABLE) == 0 ?
		    ENOTTY : scsipi_start(periph, SSS_STOP|SSS_LOEJ, 0));

	case DIOCGDEFLABEL:
		sdgetdefaultlabel(sd, (struct disklabel *)addr);
		return (0);

	case DIOCGCACHE:
		return (sd_getcache(sd, (int *) addr));

	case DIOCSCACHE:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		return (sd_setcache(sd, *(int *) addr));

	case DIOCCACHESYNC:
		/*
		 * XXX Do we really need to care about having a writable
		 * file descriptor here?
		 */
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (((sd->flags & SDF_DIRTY) != 0 || *(int *)addr != 0)) {
			error = sd_flush(sd, 0);
			if (error)
				sd->flags &= ~SDF_FLUSHING;
			else
				sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
		} else
			error = 0;
		return (error);

	default:
		if (part != RAW_PART) {
			return (ENOTTY);
		}
		return (scsipi_do_ioctl(periph, dev, cmd, addr, flag, p));
	}

#ifdef DIAGNOSTIC
	panic("sdioctl: impossible");
#endif
}

/*
 * Perform special action on behalf of the user
 * Knows about the internals of this device
 */
int
sdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct sd_softc *sd;
	int error;

	sd = sd_cd.cd_devs[SDUNIT(dev)];

	error = sdioctl_sc(sd, dev, cmd, addr, flag, p);
	if(error != 0) {
		error = ioctldisklabel(&sd->sc_dk, sdstrategy, dev, cmd, addr, flag);
	}
	return (error);
}

void
sdgetdefaultlabel(sd, lp)
	struct sd_softc *sd;
	struct disklabel *lp;
{

	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize = sd->params.blksize;
	lp->d_ntracks = sd->params.heads;
	lp->d_nsectors = sd->params.sectors;
	lp->d_ncylinders = sd->params.cyls;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	switch (scsipi_periph_bustype(sd->sc_periph)) {
	case SCSIPI_BUSTYPE_SCSI:
		lp->d_type = DTYPE_SCSI;
		break;
	case SCSIPI_BUSTYPE_ATAPI:
		lp->d_type = DTYPE_ATAPI;
		break;
	}
	/*
	 * XXX
	 * We could probe the mode pages to figure out what kind of disc it is.
	 * Is this worthwhile?
	 */
	strncpy(lp->d_typename, "mydisk", 16);
	strncpy(lp->d_packname, "fictitious", 16);
	lp->d_secperunit = sd->params.disksize;
	lp->d_rpm = sd->params.rot_rate;
	lp->d_interleave = 1;
	lp->d_flags = sd->sc_periph->periph_flags & PERIPH_REMOVABLE ?
	    D_REMOVABLE : 0;

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
 * Load the label information on the named device
 */
void
sdgetdisklabel(sd)
	struct sd_softc *sd;
{
	struct disklabel *lp = sd->sc_dk.dk_label;
	const char *errstring;

	memset(sd->sc_dk.dk_cpulabel, 0, sizeof(struct cpu_disklabel));

	sdgetdefaultlabel(sd, lp);

	if (lp->d_secpercyl == 0) {
		lp->d_secpercyl = 100;
		/* as long as it's not 0 - readdisklabel divides by it (?) */
	}

	/*
	 * Call the generic disklabel extraction routine
	 */
	errstring = readdisklabel(MAKESDDEV(0, sd->sc_dev.dv_unit, RAW_PART),
	    sdstrategy, lp);
	if (errstring) {
		printf("%s: %s\n", sd->sc_dev.dv_xname, errstring);
		return;
	}
}

void
sd_shutdown(arg)
	void *arg;
{
	struct sd_softc *sd = arg;

	/*
	 * If the disk cache needs to be flushed, and the disk supports
	 * it, flush it.  We're cold at this point, so we poll for
	 * completion.
	 */
	if ((sd->flags & SDF_DIRTY) != 0) {
		if (sd_flush(sd, XS_CTL_NOSLEEP|XS_CTL_POLL)) {
			printf("%s: cache synchronization failed\n",
			    sd->sc_dev.dv_xname);
			sd->flags &= ~SDF_FLUSHING;
		} else
			sd->flags &= ~(SDF_FLUSHING|SDF_DIRTY);
	}
}

/*
 * Check Errors
 */
int
sd_interpret_sense(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_periph *periph = xs->xs_periph;
	struct scsipi_sense_data *sense = &xs->sense.scsi_sense;
	struct sd_softc *sd = (void *)periph->periph_dev;
	int s, error, retval = EJUSTRETURN;

	/*
	 * If the periph is already recovering, just do the normal
	 * error processing.
	 */
	if (periph->periph_flags & PERIPH_RECOVERING)
		return (retval);

	/*
	 * If the device is not open yet, let the generic code handle it.
	 */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		return (retval);

	/*
	 * If it isn't a extended or extended/deferred error, let
	 * the generic code handle it.
	 */
	if ((sense->error_code & SSD_ERRCODE) != 0x70 &&
	    (sense->error_code & SSD_ERRCODE) != 0x71)
		return (retval);

	if ((sense->flags & SSD_KEY) == SKEY_NOT_READY &&
	    sense->add_sense_code == 0x4) {
		if (sense->add_sense_code_qual == 0x01)	{
			/*
			 * Unit In The Process Of Becoming Ready.
			 */
			printf("%s: waiting for pack to spin up...\n",
			    sd->sc_dev.dv_xname);
			if (!callout_pending(&periph->periph_callout))
				scsipi_periph_freeze(periph, 1);
			callout_reset(&periph->periph_callout,
			    5 * hz, scsipi_periph_timed_thaw, periph);
			retval = ERESTART;
		} else if (sense->add_sense_code_qual == 0x02) {
			printf("%s: pack is stopped, restarting...\n",
			    sd->sc_dev.dv_xname);
			s = splbio();
			periph->periph_flags |= PERIPH_RECOVERING;
			splx(s);
			error = scsipi_start(periph, SSS_START,
			    XS_CTL_URGENT|XS_CTL_HEAD_TAG|
			    XS_CTL_THAW_PERIPH|XS_CTL_FREEZE_PERIPH);
			if (error) {
				printf("%s: unable to restart pack\n",
				    sd->sc_dev.dv_xname);
				retval = error;
			} else
				retval = ERESTART;
			s = splbio();
			periph->periph_flags &= ~PERIPH_RECOVERING;
			splx(s);
		}
	}
	if ((sense->flags & SSD_KEY) == SKEY_MEDIUM_ERROR &&
	    sense->add_sense_code == 0x31 &&
	    sense->add_sense_code_qual == 0x00)	{ /* maybe for any asq ? */
		/* Medium Format Corrupted */
		retval = EFTYPE;
	}
	return (retval);
}


int
sdsize(dev)
	dev_t dev;
{
	struct sd_softc *sd;
	int part, unit, omask;
	int size;

	unit = SDUNIT(dev);
	if (unit >= sd_cd.cd_ndevs)
		return (-1);
	sd = sd_cd.cd_devs[unit];
	if (sd == NULL)
		return (-1);

	if ((sd->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (-1);

	part = SDPART(dev);
	omask = sd->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && sdopen(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	if ((sd->sc_periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		size = -1;
	else if (sd->sc_dk.dk_label->d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = sd->sc_dk.dk_label->d_partitions[part].p_size *
		    (sd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && sdclose(dev, 0, S_IFBLK, NULL) != 0)
		return (-1);
	return (size);
}

/* #define SD_DUMP_NOT_TRUSTED if you just want to watch */
static struct scsipi_xfer sx;
static int sddoingadump;

/*
 * dump all of physical memory into the partition specified, starting
 * at offset 'dumplo' into the partition.
 */
int
sddump(dev, blkno, va, size)
	dev_t dev;
	daddr_t blkno;
	caddr_t va;
	size_t size;
{
	struct sd_softc *sd;	/* disk unit to do the I/O */
	struct disklabel *lp;	/* disk's disklabel */
	int	unit, part;
	int	sectorsize;	/* size of a disk sector */
	int	nsects;		/* number of sectors in partition */
	int	sectoff;	/* sector offset of partition */
	int	totwrt;		/* total number of sectors left to write */
	int	nwrt;		/* current number of sectors to write */
	struct scsipi_rw_big cmd;	/* write command */
	struct scsipi_xfer *xs;	/* ... convenience */
	struct scsipi_periph *periph;
	struct scsipi_channel *chan;

	/* Check if recursive dump; if so, punt. */
	if (sddoingadump)
		return (EFAULT);

	/* Mark as active early. */
	sddoingadump = 1;

	unit = SDUNIT(dev);	/* Decompose unit & partition. */
	part = SDPART(dev);

	/* Check for acceptable drive number. */
	if (unit >= sd_cd.cd_ndevs || (sd = sd_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if ((sd->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (ENODEV);

	periph = sd->sc_periph;
	chan = periph->periph_channel;

	/* Make sure it was initialized. */
	if ((periph->periph_flags & PERIPH_MEDIA_LOADED) == 0)
		return (ENXIO);

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = sd->sc_dk.dk_label;
	sectorsize = lp->d_secsize;
	if ((size % sectorsize) != 0)
		return (EFAULT);
	totwrt = size / sectorsize;
	blkno = dbtob(blkno) / sectorsize;	/* blkno in DEV_BSIZE units */

	nsects = lp->d_partitions[part].p_size;
	sectoff = lp->d_partitions[part].p_offset;

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + totwrt) > nsects))
		return (EINVAL);

	/* Offset block number to start of partition. */
	blkno += sectoff;

	xs = &sx;

	while (totwrt > 0) {
		nwrt = totwrt;		/* XXX */
#ifndef	SD_DUMP_NOT_TRUSTED
		/*
		 *  Fill out the scsi command
		 */
		memset(&cmd, 0, sizeof(cmd));
		cmd.opcode = WRITE_BIG;
		_lto4b(blkno, cmd.addr);
		_lto2b(nwrt, cmd.length);
		/*
		 * Fill out the scsipi_xfer structure
		 *    Note: we cannot sleep as we may be an interrupt
		 * don't use scsipi_command() as it may want to wait
		 * for an xs.
		 */
		memset(xs, 0, sizeof(sx));
		xs->xs_control |= XS_CTL_NOSLEEP | XS_CTL_POLL |
		    XS_CTL_DATA_OUT;
		xs->xs_status = 0;
		xs->xs_periph = periph;
		xs->xs_retries = SDRETRIES;
		xs->timeout = 10000;	/* 10000 millisecs for a disk ! */
		xs->cmd = (struct scsipi_generic *)&cmd;
		xs->cmdlen = sizeof(cmd);
		xs->resid = nwrt * sectorsize;
		xs->error = XS_NOERROR;
		xs->bp = 0;
		xs->data = va;
		xs->datalen = nwrt * sectorsize;

		/*
		 * Pass all this info to the scsi driver.
		 */
		scsipi_adapter_request(chan, ADAPTER_REQ_RUN_XFER, xs);
		if ((xs->xs_status & XS_STS_DONE) == 0 ||
		    xs->error != XS_NOERROR)
			return (EIO);
#else	/* SD_DUMP_NOT_TRUSTED */
		/* Let's just talk about this first... */
		printf("sd%d: dump addr 0x%x, blk %d\n", unit, va, blkno);
		delay(500 * 1000);	/* half a second */
#endif	/* SD_DUMP_NOT_TRUSTED */

		/* update block count */
		totwrt -= nwrt;
		blkno += nwrt;
		va += sectorsize * nwrt;
	}
	sddoingadump = 0;
	return (0);
}

int
sd_mode_sense(sd, byte2, sense, size, page, flags, big)
	struct sd_softc *sd;
	u_int8_t byte2;
	void *sense;
	size_t size;
	int page, flags;
	int *big;
{

	if ((sd->sc_periph->periph_quirks & PQUIRK_ONLYBIG) &&
	    !(sd->sc_periph->periph_quirks & PQUIRK_NOBIGMODESENSE)) {
		*big = 1;
		return scsipi_mode_sense_big(sd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsipi_mode_header_big),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	} else {
		*big = 0;
		return scsipi_mode_sense(sd->sc_periph, byte2, page, sense,
		    size + sizeof(struct scsipi_mode_header),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	}
}

int
sd_mode_select(sd, byte2, sense, size, flags, big)
	struct sd_softc *sd;
	u_int8_t byte2;
	void *sense;
	size_t size;
	int flags, big;
{

	if (big) {
		struct scsipi_mode_header_big *header = sense;

		_lto2b(0, header->data_length);
		return scsipi_mode_select_big(sd->sc_periph, byte2, sense,
		    size + sizeof(struct scsipi_mode_header_big),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	} else {
		struct scsipi_mode_header *header = sense;

		header->data_length = 0;
		return scsipi_mode_select(sd->sc_periph, byte2, sense,
		    size + sizeof(struct scsipi_mode_header),
		    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);
	}
}

int
sd_get_simplifiedparms(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct {
		struct scsipi_mode_header header;
		/* no block descriptor */
		u_int8_t pg_code; /* page code (should be 6) */
		u_int8_t pg_length; /* page length (should be 11) */
		u_int8_t wcd; /* bit0: cache disable */
		u_int8_t lbs[2]; /* logical block size */
		u_int8_t size[5]; /* number of log. blocks */
		u_int8_t pp; /* power/performance */
		u_int8_t flags;
		u_int8_t resvd;
	} scsipi_sense;
	u_int64_t sectors;
	int error;

	/*
	 * scsipi_size (ie "read capacity") and mode sense page 6
	 * give the same information. Do both for now, and check
	 * for consistency.
	 * XXX probably differs for removable media
	 */
	dp->blksize = 512;
	if ((sectors = scsipi_size(sd->sc_periph, flags)) == 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	error = scsipi_mode_sense(sd->sc_periph, SMS_DBD, 6,
	    &scsipi_sense.header, sizeof(scsipi_sense),
	    flags | XS_CTL_DATA_ONSTACK, SDRETRIES, 6000);

	if (error != 0)
		return (SDGP_RESULT_OFFLINE);		/* XXX? */

	dp->blksize = _2btol(scsipi_sense.lbs);
	if (dp->blksize == 0)
		dp->blksize = 512;

	/*
	 * Create a pseudo-geometry.
	 */
	dp->heads = 64;
	dp->sectors = 32;
	dp->cyls = sectors / (dp->heads * dp->sectors);
	dp->disksize = _5btol(scsipi_sense.size);
	if (dp->disksize <= UINT32_MAX && dp->disksize != sectors) {
		printf("RBC size: mode sense=%llu, get cap=%llu\n",
		       (unsigned long long)dp->disksize,
		       (unsigned long long)sectors);
		dp->disksize = sectors;
	}
	dp->disksize512 = (dp->disksize * dp->blksize) / DEV_BSIZE;

	return (SDGP_RESULT_OK);
}

/*
 * Get the scsi driver to send a full inquiry to the * device and use the
 * results to fill out the disk parameter structure.
 */
int
sd_get_capacity(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	u_int64_t sectors;
	int error;
#if 0
	int i;
	u_int8_t *p;
#endif

	dp->disksize = sectors = scsipi_size(sd->sc_periph, flags);
	if (sectors == 0) {
		struct scsipi_read_format_capacities cmd;
		struct {
			struct scsipi_capacity_list_header header;
			struct scsipi_capacity_descriptor desc;
		} __attribute__((packed)) data;

		memset(&cmd, 0, sizeof(cmd));
		memset(&data, 0, sizeof(data));
		cmd.opcode = READ_FORMAT_CAPACITIES;
		_lto2b(sizeof(data), cmd.length);

		error = scsipi_command(sd->sc_periph, NULL,
		    (void *)&cmd, sizeof(cmd), (void *)&data, sizeof(data),
		    SDRETRIES, 20000, NULL,
		    flags | XS_CTL_DATA_IN | XS_CTL_DATA_ONSTACK);
		if (error == EFTYPE) {
			/* Medium Format Corrupted, handle as not formatted */
			return (SDGP_RESULT_UNFORMATTED);
		}
		if (error || data.header.length == 0)
			return (SDGP_RESULT_OFFLINE);

#if 0
printf("rfc: length=%d\n", data.header.length);
printf("rfc result:"); for (i = sizeof(struct scsipi_capacity_list_header) + data.header.length, p = (void *)&data; i; i--, p++) printf(" %02x", *p); printf("\n");
#endif
		switch (data.desc.byte5 & SCSIPI_CAP_DESC_CODE_MASK) {
		case SCSIPI_CAP_DESC_CODE_RESERVED:
		case SCSIPI_CAP_DESC_CODE_FORMATTED:
			break;

		case SCSIPI_CAP_DESC_CODE_UNFORMATTED:
			return (SDGP_RESULT_UNFORMATTED);

		case SCSIPI_CAP_DESC_CODE_NONE:
			return (SDGP_RESULT_OFFLINE);
		}

		dp->disksize = sectors = _4btol(data.desc.nblks);
		if (sectors == 0)
			return (SDGP_RESULT_OFFLINE);		/* XXX? */

		dp->blksize = _3btol(data.desc.blklen);
		if (dp->blksize == 0)
			dp->blksize = 512;
	} else {
		struct sd_mode_sense_data scsipi_sense;
		int big, bsize;
		struct scsi_blk_desc *bdesc;

		memset(&scsipi_sense, 0, sizeof(scsipi_sense));
		error = sd_mode_sense(sd, 0, &scsipi_sense,
		    sizeof(scsipi_sense.blk_desc), 0, flags | XS_CTL_SILENT, &big);
		dp->blksize = 512;
		if (!error) {
			if (big) {
				bdesc = (void *)(&scsipi_sense.header.big + 1);
				bsize = _2btol(scsipi_sense.header.big.blk_desc_len);
			} else {
				bdesc = (void *)(&scsipi_sense.header.small + 1);
				bsize = scsipi_sense.header.small.blk_desc_len;
			}

#if 0
printf("page 0 sense:"); for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i; i--, p++) printf(" %02x", *p); printf("\n");
printf("page 0 bsize=%d\n", bsize);
printf("page 0 ok\n");
#endif

			if (bsize >= 8) {
				dp->blksize = _3btol(bdesc->blklen);
				if (dp->blksize == 0)
					dp->blksize = 512;
			}
		}
	}

	dp->disksize512 = (sectors * dp->blksize) / DEV_BSIZE;
	return (0);
}

int
sd_get_parms_page4(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct sd_mode_sense_data scsipi_sense;
	int error;
	int big, poffset, byte2;
	union scsi_disk_pages *pages;
#if 0
	int i;
	u_int8_t *p;
#endif

	byte2 = SMS_DBD;
again:
	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, byte2, &scsipi_sense,
	    (byte2 ? 0 : sizeof(scsipi_sense.blk_desc)) +
	    sizeof(scsipi_sense.pages.rigid_geometry), 4,
	    flags | XS_CTL_SILENT, &big);
	if (error) {
		if (byte2 == SMS_DBD) {
			/* No result; try once more with DBD off */
			byte2 = 0;
			goto again;
		}
		return (error);
	}

	if (big) {
		poffset = sizeof scsipi_sense.header.big;
		poffset += _2btol(scsipi_sense.header.big.blk_desc_len);
	} else {
		poffset = sizeof scsipi_sense.header.small;
		poffset += scsipi_sense.header.small.blk_desc_len;
	}

	pages = (void *)((u_long)&scsipi_sense + poffset);
#if 0
printf("page 4 sense:"); for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i; i--, p++) printf(" %02x", *p); printf("\n");
printf("page 4 pg_code=%d sense=%p/%p\n", pages->rigid_geometry.pg_code, &scsipi_sense, pages);
#endif

	if ((pages->rigid_geometry.pg_code & PGCODE_MASK) != 4)
		return (ERESTART);

	SC_DEBUG(sd->sc_periph, SCSIPI_DB3,
	    ("%d cyls, %d heads, %d precomp, %d red_write, %d land_zone\n",
	    _3btol(pages->rigid_geometry.ncyl),
	    pages->rigid_geometry.nheads,
	    _2btol(pages->rigid_geometry.st_cyl_wp),
	    _2btol(pages->rigid_geometry.st_cyl_rwc),
	    _2btol(pages->rigid_geometry.land_zone)));

	/*
	 * KLUDGE!! (for zone recorded disks)
	 * give a number of sectors so that sec * trks * cyls
	 * is <= disk_size
	 * can lead to wasted space! THINK ABOUT THIS !
	 */
	dp->heads = pages->rigid_geometry.nheads;
	dp->cyls = _3btol(pages->rigid_geometry.ncyl);
	if (dp->heads == 0 || dp->cyls == 0)
		return (ERESTART);
	dp->sectors = dp->disksize / (dp->heads * dp->cyls);	/* XXX */

	dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
	if (dp->rot_rate == 0)
		dp->rot_rate = 3600;

#if 0
printf("page 4 ok\n");
#endif
	return (0);
}

int
sd_get_parms_page5(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	struct sd_mode_sense_data scsipi_sense;
	int error;
	int big, poffset, byte2;
	union scsi_disk_pages *pages;
#if 0
	int i;
	u_int8_t *p;
#endif

	byte2 = SMS_DBD;
again:
	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, 0, &scsipi_sense,
	    (byte2 ? 0 : sizeof(scsipi_sense.blk_desc)) +
	    sizeof(scsipi_sense.pages.flex_geometry), 5,
	    flags | XS_CTL_SILENT, &big);
	if (error) {
		if (byte2 == SMS_DBD) {
			/* No result; try once more with DBD off */
			byte2 = 0;
			goto again;
		}
		return (error);
	}

	if (big) {
		poffset = sizeof scsipi_sense.header.big;
		poffset += _2btol(scsipi_sense.header.big.blk_desc_len);
	} else {
		poffset = sizeof scsipi_sense.header.small;
		poffset += scsipi_sense.header.small.blk_desc_len;
	}

	pages = (void *)((u_long)&scsipi_sense + poffset);
#if 0
printf("page 5 sense:"); for (i = sizeof(scsipi_sense), p = (void *)&scsipi_sense; i; i--, p++) printf(" %02x", *p); printf("\n");
printf("page 5 pg_code=%d sense=%p/%p\n", pages->flex_geometry.pg_code, &scsipi_sense, pages);
#endif

	if ((pages->flex_geometry.pg_code & PGCODE_MASK) != 5)
		return (ERESTART);

	SC_DEBUG(sd->sc_periph, SCSIPI_DB3,
	    ("%d cyls, %d heads, %d sec, %d bytes/sec\n",
	    _3btol(pages->flex_geometry.ncyl),
	    pages->flex_geometry.nheads,
	    pages->flex_geometry.ph_sec_tr,
	    _2btol(pages->flex_geometry.bytes_s)));

	dp->heads = pages->flex_geometry.nheads;
	dp->cyls = _2btol(pages->flex_geometry.ncyl);
	dp->sectors = pages->flex_geometry.ph_sec_tr;
	if (dp->heads == 0 || dp->cyls == 0 || dp->sectors == 0)
		return (ERESTART);

	dp->rot_rate = _2btol(pages->rigid_geometry.rpm);
	if (dp->rot_rate == 0)
		dp->rot_rate = 3600;

#if 0
printf("page 5 ok\n");
#endif
	return (0);
}

int
sd_get_parms(sd, dp, flags)
	struct sd_softc *sd;
	struct disk_parms *dp;
	int flags;
{
	int error;

	/*
	 * If offline, the SDEV_MEDIA_LOADED flag will be
	 * cleared by the caller if necessary.
	 */
	if (sd->type == T_SIMPLE_DIRECT)
		return (sd_get_simplifiedparms(sd, dp, flags));

	error = sd_get_capacity(sd, dp, flags);
	if (error)
		return (error);

	if (sd->type == T_OPTICAL)
		goto page0;

	if (sd->sc_periph->periph_flags & PERIPH_REMOVABLE) {
		if (!sd_get_parms_page5(sd, dp, flags) ||
		    !sd_get_parms_page4(sd, dp, flags))
			return (SDGP_RESULT_OK);
	} else {
		if (!sd_get_parms_page4(sd, dp, flags) ||
		    !sd_get_parms_page5(sd, dp, flags))
			return (SDGP_RESULT_OK);
	}

page0:
	printf("%s: fabricating a geometry\n", sd->sc_dev.dv_xname);
	/* Try calling driver's method for figuring out geometry. */
	if (!sd->sc_periph->periph_channel->chan_adapter->adapt_getgeom ||
	    !(*sd->sc_periph->periph_channel->chan_adapter->adapt_getgeom)
		(sd->sc_periph, dp, dp->disksize)) {
		/*
		 * Use adaptec standard fictitious geometry
		 * this depends on which controller (e.g. 1542C is
		 * different. but we have to put SOMETHING here..)
		 */
		dp->heads = 64;
		dp->sectors = 32;
		dp->cyls = dp->disksize / (64 * 32);
	}
	dp->rot_rate = 3600;
	return (SDGP_RESULT_OK);
}

int
sd_flush(sd, flags)
	struct sd_softc *sd;
	int flags;
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct scsi_synchronize_cache cmd;

	/*
	 * If the device is SCSI-2, issue a SYNCHRONIZE CACHE.
	 * We issue with address 0 length 0, which should be
	 * interpreted by the device as "all remaining blocks
	 * starting at address 0".  We ignore ILLEGAL REQUEST
	 * in the event that the command is not supported by
	 * the device, and poll for completion so that we know
	 * that the cache has actually been flushed.
	 *
	 * Unless, that is, the device can't handle the SYNCHRONIZE CACHE
	 * command, as indicated by our quirks flags.
	 *
	 * XXX What about older devices?
	 */
	if (periph->periph_version < 2 ||
	    (periph->periph_quirks & PQUIRK_NOSYNCCACHE))
		return (0);

	sd->flags |= SDF_FLUSHING;
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_SYNCHRONIZE_CACHE;

	return (scsipi_command(periph, NULL, (void *)&cmd, sizeof(cmd), 0, 0,
	    SDRETRIES, 100000, NULL, flags | XS_CTL_IGNORE_ILLEGAL_REQUEST));
}

int
sd_getcache(sd, bitsp)
	struct sd_softc *sd;
	int *bitsp;
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct sd_mode_sense_data scsipi_sense;
	int error, bits = 0;
	int big;
	union scsi_disk_pages *pages;

	if (periph->periph_version < 2)
		return (EOPNOTSUPP);

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params), 8, 0, &big);
	if (error)
		return (error);

	if (big)
		pages = (void *)(&scsipi_sense.header.big + 1);
	else
		pages = (void *)(&scsipi_sense.header.small + 1);

	if ((pages->caching_params.flags & CACHING_RCD) == 0)
		bits |= DKCACHE_READ;
	if (pages->caching_params.flags & CACHING_WCE)
		bits |= DKCACHE_WRITE;
	if (pages->caching_params.pg_code & PGCODE_PS)
		bits |= DKCACHE_SAVE;

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params),
	    SMS_PAGE_CTRL_CHANGEABLE|8, 0, &big);
	if (error == 0) {
		if (big)
			pages = (void *)(&scsipi_sense.header.big + 1);
		else
			pages = (void *)(&scsipi_sense.header.small + 1);

		if (pages->caching_params.flags & CACHING_RCD)
			bits |= DKCACHE_RCHANGE;
		if (pages->caching_params.flags & CACHING_WCE)
			bits |= DKCACHE_WCHANGE;
	}

	*bitsp = bits;

	return (0);
}

int
sd_setcache(sd, bits)
	struct sd_softc *sd;
	int bits;
{
	struct scsipi_periph *periph = sd->sc_periph;
	struct sd_mode_sense_data scsipi_sense;
	int error;
	uint8_t oflags, byte2 = 0;
	int big;
	union scsi_disk_pages *pages;

	if (periph->periph_version < 2)
		return (EOPNOTSUPP);

	memset(&scsipi_sense, 0, sizeof(scsipi_sense));
	error = sd_mode_sense(sd, SMS_DBD, &scsipi_sense,
	    sizeof(scsipi_sense.pages.caching_params), 8, 0, &big);
	if (error)
		return (error);

	if (big)
		pages = (void *)(&scsipi_sense.header.big + 1);
	else
		pages = (void *)(&scsipi_sense.header.small + 1);

	oflags = pages->caching_params.flags;

	if (bits & DKCACHE_READ)
		pages->caching_params.flags &= ~CACHING_RCD;
	else
		pages->caching_params.flags |= CACHING_RCD;

	if (bits & DKCACHE_WRITE)
		pages->caching_params.flags |= CACHING_WCE;
	else
		pages->caching_params.flags &= ~CACHING_WCE;

	if (oflags == pages->caching_params.flags)
		return (0);

	pages->caching_params.pg_code &= PGCODE_MASK;

	if (bits & DKCACHE_SAVE)
		byte2 |= SMS_SP;

	return (sd_mode_select(sd, byte2|SMS_PF, &scsipi_sense,
	    sizeof(struct scsipi_mode_page_header) +
	    pages->caching_params.pg_length, 0, big));
}
