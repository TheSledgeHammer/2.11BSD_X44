/*	$NetBSD: stvar.h,v 1.5 2002/03/20 14:54:00 christos Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 * Originally written by Julian Elischer (julian@tfs.com)
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
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 * major changes by Julian Elischer (julian@jules.dialix.oz.au) May 1993
 *
 * A lot of rewhacking done by mjacob (mjacob@nas.nasa.gov).
 */

#define	ST_IO_TIME	(3 * 60 * 1000)			/* 3 minutes */
#define	ST_CTL_TIME	(30 * 1000)				/* 30 seconds */
#define	ST_SPC_TIME	(4 * 60 * 60 * 1000)	/* 4 hours */

/*
 * Define various devices that we know mis-behave in some way,
 * and note how they are bad, so we can correct for them
 */
struct modes {
	u_int 		quirks;				/* same definitions as in quirkdata */
	int 		blksize;
	u_int8_t 	density;
};

struct quirkdata {
	u_int 		quirks;
#define	ST_Q_FORCE_BLKSIZE	0x0001
#define	ST_Q_SENSE_HELP		0x0002	/* must do READ for good MODE SENSE */
#define	ST_Q_IGNORE_LOADS	0x0004
#define	ST_Q_BLKSIZE		0x0008	/* variable-block media_blksize > 0 */
#define	ST_Q_UNIMODAL		0x0010	/* unimode drive rejects mode select */
#define	ST_Q_NOPREVENT		0x0020	/* does not support PREVENT */
#define	ST_Q_ERASE_NOIMM	0x0040	/* drive rejects ERASE/w Immed bit */
#define	ST_Q_NOFILEMARKS	0x0080	/* can only write 0 filemarks */
	u_int 		page_0_size;
#define	MAX_PAGE_0_SIZE		64
	struct modes modes[4];
};

struct st_quirk_inquiry_pattern {
	struct scsi_inquiry_pattern pattern;
	struct quirkdata 			quirkdata;
};

struct st_softc {
	struct device 		sc_dev;
/*--------------------present operating parameters, flags etc.----------------*/
	int 				flags;			/* see below                          */
	u_int 				quirks;			/* quirks for the open mode           */
	int 				blksize;		/* blksize we are using                */
	u_int8_t 			density;		/* present density                    */
	u_int 				page_0_size;	/* size of page 0 data		      */
	u_int 				last_dsty;		/* last density opened               */
/*--------------------device/scsi parameters----------------------------------*/
	struct scsi_link 	*sc_link;		/* our link to the adpter etc.        */
/*--------------------parameters reported by the device ----------------------*/
	int 				blkmin;			/* min blk size                       */
	int 				blkmax;			/* max blk size                       */
	struct quirkdata 	*quirkdata;		/* if we have a rogue entry           */
/*--------------------parameters reported by the device for this media--------*/
	u_long 				numblks;		/* nominal blocks capacity            */
	int 				media_blksize;	/* 0 if not ST_FIXEDBLOCKS            */
	u_int8_t 			media_density;	/* this is what it said when asked    */
/*--------------------quirks for the whole drive------------------------------*/
	u_int 				drive_quirks;	/* quirks of this drive               */
/*--------------------How we should set up when opening each minor device----*/
	struct modes 		modes[4];		/* plus more for each mode            */
	u_int8_t  			modeflags[4];	/* flags for the modes                */
#define DENSITY_SET_BY_USER		0x01
#define DENSITY_SET_BY_QUIRK	0x02
#define BLKSIZE_SET_BY_USER		0x04
#define BLKSIZE_SET_BY_QUIRK	0x08
/*--------------------storage for sense data returned by the drive------------*/
	u_char 				sense_data[MAX_PAGE_0_SIZE];	/*
						 	 	 	 	 	 	 * additional sense data needed
						 	 	 	 	 	 	 * for mode sense/select.
						 	 	 	 	 	 	 */
	struct buf 			buf_queue;		/* the queue of pending IO operations */
};

#define	ST_INFO_VALID	0x0001
#define	ST_BLOCK_SET	0x0002	/* block size, mode set by ioctl      */
#define	ST_WRITTEN		0x0004	/* data have been written, EOD needed */
#define	ST_FIXEDBLOCKS	0x0008
#define	ST_AT_FILEMARK	0x0010
#define	ST_EIO_PENDING	0x0020	/* we couldn't report it then (had data) */
#define	ST_NEW_MOUNT	0x0040	/* still need to decide mode              */
#define	ST_READONLY		0x0080	/* st_mode_sense says write protected */
#define	ST_FM_WRITTEN	0x0100	/*
				 	 	 	 	 * EOF file mark written  -- used with
				 	 	 	 	 * ~ST_WRITTEN to indicate that multiple file
				 	 	 	 	 * marks have been written
				 	 	 	 	 */
#define	ST_BLANK_READ	0x0200	/* BLANK CHECK encountered already */
#define	ST_2FM_AT_EOD	0x0400	/* write 2 file marks at EOD */
#define	ST_MOUNTED		0x0800	/* Device is presently mounted */
#define	ST_DONTBUFFER	0x1000	/* Disable buffering/caching */

#define	ST_PER_ACTION	(ST_AT_FILEMARK | ST_EIO_PENDING | ST_BLANK_READ)
#define	ST_PER_MOUNT	(ST_INFO_VALID | ST_BLOCK_SET | ST_WRITTEN | 		\
			 	 	 	 ST_FIXEDBLOCKS | ST_READONLY | ST_FM_WRITTEN | 	\
						 ST_2FM_AT_EOD | ST_PER_ACTION)

void	stattach (struct device *, struct st_softc *, void *);
int 	stactivate (struct device *, enum devact);
int 	stdetach (struct device *, int);
