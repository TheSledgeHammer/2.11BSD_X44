/*-
 * Copyright (c) 1994 Bruce D. Evans.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef	_SYS_DISKSLICE_H_
#define	_SYS_DISKSLICE_H_

#include <sys/reboot.h>

struct diskslice {
	u_long					ds_offset;				/* starting sector */
	u_long					ds_size;				/* number of sectors */
	int						ds_type;				/* (foreign) slice type */
	struct disklabel 		*ds_label;				/* BSD label, if any */
	u_char					ds_openmask;			/* devs open */
	u_char					ds_wlabel;				/* nonzero if label is writable */
};

struct diskslices {
	int						dss_first_bsd_slice;	/* COMPATIBILITY_SLICE is mapped here */
	u_int					dss_nslices;			/* actual dimension of dss_slices[] */
	u_int					dss_oflags;				/* copy of flags for "first" open */
	int						dss_secmult;			/* block to sector multiplier */
	int						dss_secshift;			/* block to sector shift (or -1) */
	int						dss_secsize;			/* sector size */
	struct diskslice 		dss_slices[MAX_SLICES];	/* actually usually less */
};

struct diskslice2 {
	struct disklabel 		*ds_label;				/* BSD label, if any */
	int						ds_secsize;				/* sector size */
	u_int					ds_nslices;				/* actual dimension of ds_slices[] */
	u_int16_t 				ds_checksum;			/* xor of data incl. slices */
	struct slice {
		u_long				dss_offset;				/* starting sector */
	} ds_slices[MAX_SLICES];
};

#define	DIOCGSLICEINFO		_IOR('d', 111, struct diskslices)
#define	DIOCSYNCSLICEINFO	_IOW('d', 112, int)

//#ifdef _KERNEL

/* Flags for dsopen(). */
#define	DSO_NOLABELS		1
#define	DSO_ONESLICE		2
#define	DSO_COMPATLABEL		4

#define	dsgetlabel(dev, ssp)			(ssp->dss_slices[dkslice(dev)].ds_label)

#define	dkmakeminor(unit, slice, part) 	(((slice) << 16) | (((unit) & 0x1e0) << 16) | (((unit) & 0x1f) << 3) | (part))
#define	dkslice(dev)					((minor(dev) >> 16) & 0x1f)
#define	dksparebits(dev)       			((minor(dev) >> 25) & 0x7f)

struct bio;
struct disklabel;


#endif /* _KERNEL */
#endif /* _SYS_DISKSLICE_H_ */
