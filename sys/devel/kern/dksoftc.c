/*
 * dksoftc.c
 *
 *  Created on: 7 Nov 2021
 *      Author: marti
 */
#include <sys/disk.h>
#include <sys/null.h>
#include <sys/malloc.h>
#include <sys/queue.h>

struct dksoftc {
	struct dkdevice 	sc_dkdev;
	struct dkdriver		sc_dkdriver;
	dev_t				sc_dev;
	char				*sc_xname;
	int					sc_flags;

	struct lock_object 	sc_iolock;
	struct bufq_state	sc_bufq;	/* buffer queue */
};

void
dk_init(dksc, dkdev)
	struct dksoftc 	*dksc;
	struct dkdevice *dkdev;
{
	memset(dksc, 0x0, sizeof(*dksc));
	dksc->sc_dkdev = dkdev;

	strlcpy(dksc->sc_xname, dkdev->dk_name, DK_XNAME_SIZE);
	dksc->sc_dkdev.dk_name = dksc->sc_xname;
	lockinit(&dksc->sc_iolock, PRIBIO, "dklk", 0, 0);
}

int
dk_open(dksc, dev, flags, fmt, p)
	struct dksoftc *dksc;
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct dkdevice *dk;
	struct dkdriver *dkr;
	struct disklabel *lp;
	int	part = dkpart(dev);
	int	pmask = 1 << part;
	int	ret = 0;
	int error;

	dk = dksc->sc_dkdev;
	dkr = disk_driver(dk, dev);
	lp = disk_label(dk, dev);

	if ((ret = lockmgr(&dksc->sc_iolock, LK_EXCLUSIVE, NULL)) != 0) {
		return (ret);
	}

	part = dkpart(dev);
	pmask = 1 << part;

	error = DKR_OPEN(dk, dev, flags, fmt, p);


}

int
dk_close(dksc, dev, flags, fmt, p)
	struct dksoftc *dksc;
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct dkdevice *dk;
	struct dkdriver *dkr;

	dk = dksc->sc_dkdev;
	dkr = disk_driver(dk, dev);
}

void
dk_strategy(dksc, bp)
	struct dksoftc 	*dksc;
	struct buf *bp;
{
	struct dkdevice *dk;
	struct dkdriver *dkr;
	int	s, error;
	int	wlabel;

	dk = dksc->sc_dkdev;
	dkr = disk_driver(dk, bp->b_dev);

	if(!(dksc->sc_flags & DKF_ALIVE)) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return;
	}
	error = partition_check(bp, dk);

	/* XXX look for some more errors, c.f. ld.c */

	bp->b_resid = bp->b_bcount;

	/* If there is nothing to do, then we are done */
	if (bp->b_bcount == 0) {
		biodone(bp);
		return;
	}

	bp->b_cylin = bp->b_blkno / (dk->dk_label->d_ntracks * dk->dk_label->d_nsectors);

	wlabel = dksc->sc_flags & (DKF_WLABEL|DKF_LABELLING);
	if (dkpart(bp->b_dev) != RAW_PART &&
	    bounds_check_with_label(&dksc->sc_dkdev, bp, wlabel) <= 0) {
		biodone(bp);
		return;
	}

	s = splbio();
	BUFQ_PUT(&dksc->sc_bufq, bp);
	dk_start(dksc, bp);
	splx(s);
	return;
}

int
dk_ioctl(dksc, dev, cmd, data, flag, p)
	struct dksoftc 	*dksc;
	dev_t dev;
	u_long cmd;
	void *data;
	int flag;
	struct proc *p;
{
	struct dkdevice *dk;
	struct dkdriver *dkr;
	int error;

	dk = dksc->sc_dkdev;
	dkr = disk_driver(dk, dev);

	error = disk_ioctl(dk, dev, cmd, data, flag, p);
	if (error != 0) {
		return (error);
	}

	error = ioctldisklabel(dk, dkr->d_strategy, dev, cmd, data, flag);
	if (error != 0) {
		return (error);
	}

	return (0);
}

void
dk_start(dksc, bp)
	struct dksoftc 	*dksc;
	struct buf 		*bp;
{
	struct dkdevice *dk;
	struct dkdriver *dkr;

	dk = dksc->sc_dkdev;
	dkr = disk_driver(dk, bp->b_dev);

	DPRINTF_FOLLOW(("dk_start(%s, %p)\n", dk->dk_name, dksc));

	/* Process the work queue */
	while ((bp = BUFQ_GET(&dksc->sc_bufq)) != NULL) {
		if (dkr->d_start(bp) != 0) {
			BUFQ_PUT(&dksc->sc_bufq, bp);
			break;
		}
	}
}

int
dk_size(dksc, dev)
	struct dksoftc 	*dksc;
	dev_t 			dev;
{
	struct dkdevice *dk;
	struct dkdriver *dkr;
	struct disklabel *lp;
	int	is_open;
	int	part;
	int	size;

	dk = dksc->sc_dkdev;
	dkr = disk_driver(dk, dev);

	if ((dksc->sc_flags & DKF_INITED) == 0) {
		return (-1);
	}

	part = dkpart(dev);
	is_open = dk->dk_openmask & (1 << part);

	if (!is_open && dkr->d_open(dev, 0, S_IFBLK, curproc())) {
		return (-1);
	}

	lp = disk_label(dk, dev);
	if (lp->d_partitions[part].p_fstype != FS_SWAP) {
		size = -1;
	} else {
		size = lp->d_partitions[part].p_size * (lp->d_secsize / DEV_BSIZE);
	}

	if (!is_open && dkr->d_close(dev, 0, S_IFBLK, curproc)) {
		return 1;
	}

	return (size);
}

#define DKF_READYFORDUMP		(DKF_INITED|DKF_TAKEDUMP)
#define DKFF_READYFORDUMP(x)	(((x) & DKF_READYFORDUMP) == DKF_READYFORDUMP)
static volatile int	dk_dumping = 0;

int
dk_dump(dksc, dev)
	struct dksoftc *dksc;
	dev_t dev;
{
	struct dkdevice *dk;
	struct dkdriver *dkr;

	dk = dksc->sc_dkdev;
	dkr = disk_driver(dk, dev);

	/*
	 * ensure that we consider this device to be safe for dumping,
	 * and that the device is configured.
	 */
	if (!DKFF_READYFORDUMP(dksc->sc_flags)) {
		return (ENXIO);
	}

	/* ensure that we are not already dumping */
	if (dk_dumping) {
		return (EFAULT);
	}
	dk_dumping = 1;

	/* XXX: unimplemented */

	dk_dumping = 0;

	/* XXX: actually for now, we are going to leave this alone */
	return (ENXIO);
}
