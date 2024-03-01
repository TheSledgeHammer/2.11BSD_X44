/*
 * Copyright (c) 2007 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@backplane.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/endian.h>
#include <sys/disk.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/diskmbr.h>
#include <sys/diskgpt.h>
#include <sys/diskslice.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/vnode.h>

static void gpt_setslice(const char *, struct dkdevice *, struct diskslice *, struct gpt_ent *);

static int
gpt_read_sector(bp, lp, dev, sector, count, msg)
	struct buf *bp;
	struct disklabel *lp;
	dev_t dev;
	u_long sector, count;
	char *msg;
{
	int	error;

	bp->b_blkno = sector;
	bp->b_bcount = count;
	bp->b_flags = (bp->b_flags & ~(B_WRITE | B_DONE)) | B_READ;
	bp->b_cylin = sector / lp->d_secpercyl;
	VOP_STRATEGY(bp);
	error = biowait(bp);
	if (error != 0) {
		diskerr(bp, devtoname(dev), msg, LOG_PRINTF, 0, (struct disklabel*) NULL);
		printf("\n");
	}
	return (error);
}

/*
 * Handle GPT on raw disk.  Note that GPTs are not recursive.  The MBR is
 * ignored once a GPT has been detected.
 *
 * GPTs always start at block #1, regardless of how the MBR has been set up.
 * In fact, the MBR's starting block might be pointing to the boot partition
 * in the GPT rather then to the start of the GPT.
 *
 * This routine is called from mbrinit() when a GPT has been detected.
 */
int
gptinit(diskp, dev, lp, sspp)
	struct dkdevice *diskp;
	dev_t	dev;
	struct disklabel *lp;
	struct diskslices **sspp;
{
	struct buf *bp1, *bp2;
	u_char	*cp;
	struct gpt_hdr *gpt;
	struct gpt_ent *ent;
	int error, i, j;
	uint32_t len;
	uint32_t entries;
	uint32_t entsz;
	uint32_t crc;
	uint32_t table_lba;
	uint32_t table_blocks;
	struct diskslice *sp;
	struct diskslices *ssp;

	/*
	 * The GPT starts in sector 1.
	 */
	i = 0;
	bp1 = geteblk((int)lp->d_secsize);
	bp1->b_dev = dkmakedev(major(dev), dkunit(dev), RAW_PART);
	error = gpt_read_sector(bp1, lp, dev, lp->d_secsize, lp->d_secsize, "reading GPT: error");
	if (error != 0) {
		error = EIO;
		goto done;
	}

	/*
	 * Header sanity check
	 */
	gpt = (void *)bp1->b_data;
	len = le32toh(gpt->hdr_size);
	if (len < GPT_MIN_HDR_SIZE || len > lp->d_secsize) {
		printf("%s: Illegal GPT header size\n", devtoname(dev));
		error = EINVAL;
		goto done;
	}

	crc = le32toh(gpt->hdr_crc_self);
	gpt->hdr_crc_self = 0;
	if (crc32(gpt, len) != crc) {
		printf("%s: GPT CRC32 did not match\n", devtoname(dev));
		error = EINVAL;
		goto done;
	}

	/*
	 * Validate the partition table and its location, then read it
	 * into a buffer.
	 */
	entries = le32toh(gpt->hdr_entries);
	entsz = le32toh(gpt->hdr_entsz);
	table_lba = le32toh(gpt->hdr_lba_table);
	table_blocks = (entries * entsz + lp->d_secsize - 1) / lp->d_secsize;
	if (entries < 1 || entries > 128 ||
	    entsz < 128 || (entsz & 7) || entsz > MAXBSIZE / entries ||
	    table_lba < 2 || table_lba + table_blocks > lp->d_secperunit) {
		printf("%s: GPT partition table is out of bounds\n", devtoname(dev));
		error = EINVAL;
		goto done;
	}

	bp2 = geteblk((int)(table_blocks * lp->d_secsize));
	bp2->b_dev = dkmakedev(major(dev), dkunit(dev), RAW_PART);
	error = gpt_read_sector(bp2, lp, dev, ((off_t)table_lba * lp->d_secsize), (table_blocks * lp->d_secsize), "reading GPT partition table: error");
	if (error != 0) {
		error = EIO;
		goto done;
	}

	/*
	 * We are passed a pointer to a minimal slices struct.  Replace
	 * it with a maximal one (128 slices + special slices).  Well,
	 * really there is only one special slice (the WHOLE_DISK_SLICE)
	 * since we use the compatibility slice for s0, but don't quibble.
	 *
	 */
	free(*sspp, M_DEVBUF);
	ssp = dsmakeslicestruct(BASE_SLICE+128, lp);
	*sspp = ssp;

	/*
	 * Create a slice for each partition.
	 */
	for (i = 0; i < (int) entries && i < 128; ++i) {
		struct gpt_ent sent;
		char partname[2];
		char *sname;

		ent = (void*) ((char*) bp2->b_data + i * entsz);
		le_uuid_dec(&ent->ent_type, &sent.ent_type);
		le_uuid_dec(&ent->ent_uuid, &sent.ent_uuid);
		sent.ent_lba_start = le64toh(ent->ent_lba_start);
		sent.ent_lba_end = le64toh(ent->ent_lba_end);
		sent.ent_attr = le64toh(ent->ent_attr);

		for (j = 0; j < nitems(ent->ent_name); ++j)
			sent.ent_name[j] = le16toh(ent->ent_name[j]);

		/*
		 * The COMPATIBILITY_SLICE is actually slice 0 (s0).  This
		 * is a bit weird becaue the whole-disk slice is #1, so
		 * slice 1 (s1) starts at BASE_SLICE.
		 */
		if (i == 0) {
			sp = &ssp->dss_slices[COMPATIBILITY_SLICE];
		} else {
			sp = &ssp->dss_slices[BASE_SLICE + i - 1];
		}
		sname = dsname(diskp, dev, dkunit(dev), WHOLE_DISK_SLICE, RAW_PART, partname);

		if (sent.ent_lba_start < table_lba + table_blocks ||
				sent.ent_lba_end >= lp->d_secperunit ||
				sent.ent_lba_start >= sent.ent_lba_end) {
			printf("%s part %d: unavailable, bad start or "
					"ending lba\n", sname, i);
		} else {
			gpt_setslice(sname, diskp, sp, &sent);
		}
	}
	ssp->dss_nslices = BASE_SLICE + i;

	error = 0;

done:
	if (bp1) {
		bp1->b_flags |= B_INVAL | B_AGE;
		brelse(bp1);
	}
	if (bp2) {
		bp2->b_flags |= B_INVAL | B_AGE;
		brelse(bp2);
	}
	if (error == EINVAL) {
		error = 0;
	}
	return (error);
}

static void
gpt_setslice(sname, diskp, sp, sent)
	const char *sname;
	struct dkdevice *diskp;
	struct diskslice *sp;
	struct gpt_ent *sent;
{
	sp->ds_offset = sent->ent_lba_start;
	sp->ds_size = sent->ent_lba_end + 1 - sent->ent_lba_start;
	sp->ds_type = 1; /* XXX */
	sp->ds_type_uuid = sent->ent_type;
	sp->ds_stor_uuid = sent->ent_uuid;
	sp->ds_reserved = 0; /* no reserved sectors */
}
