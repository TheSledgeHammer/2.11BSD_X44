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

#ifndef _KERNEL
#include <sys/types.h>
#include <sys/null.h>
#endif

#include <sys/reboot.h>
#include <sys/uuid.h>

struct diskslice {
	u_long					ds_offset;				/* starting sector */
	u_long					ds_size;				/* number of sectors */
	int						ds_type;				/* (foreign) slice type */
	struct disklabel 		*ds_label;				/* BSD label, if any */
	u_char					ds_openmask;			/* devs open */
	u_char					ds_wlabel;				/* nonzero if label is writable */
	u_char					ds_klabel;
	struct uuid				ds_type_uuid;			/* slice type uuid */
	struct uuid				ds_stor_uuid;			/* slice storage unique uuid */
	u_int32_t				ds_reserved;			/* sectors reserved parent overlap */
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

#define	DIOCGSLICEINFO		_IOR('d', 111, struct diskslices)
#define	DIOCSYNCSLICEINFO	_IOW('d', 112, int)

#ifdef _KERNEL

/* Flags for dsopen(). */
#define	DSO_NOLABELS		1
#define	DSO_ONESLICE		2
#define	DSO_COMPATLABEL		4

#define	dsgetlabel(dev, ssp)			(ssp->dss_slices[dkslice(dev)].ds_label)

#define	dkmakeminor(unit, slice, part) 	(((slice) << 16) | (((unit) & 0x1e0) << 16) | (((unit) & 0x1f) << 3) | (part))
#define	dkslice(dev)					((minor(dev) >> 16) & 0x1f)
#define	dksparebits(dev)       			((minor(dev) >> 25) & 0x7f)

static __inline dev_t
dkmodpart(dev_t dev, int part)
{
	return (makedev(major(dev), (minor(dev) & ~7) | part));
}

static __inline dev_t
dkmodslice(dev_t dev, int slice)
{
	return (makedev(major(dev), (minor(dev) & ~0x1f0000) | (slice << 16)));
}

struct buf;
struct disklabel;

dev_t 				makediskslice(dev_t, int, int, int);
int					dscheck(struct buf *, struct diskslices *);
void 				dsclose(dev_t, int, struct diskslices *);
void 				dsgone(struct diskslices **);
int					dsioctl(dev_t, u_long, caddr_t, int, struct diskslices **);
int					dsisopen(struct diskslices *);
struct diskslices 	*dsmakeslicestruct(int, struct disklabel *);
char				*dsname(struct dkdevice *, dev_t, int, int, int, char *);
int					dsopen(struct dkdevice *, dev_t, int, u_int, struct disklabel *);
int					dssize(struct dkdevice *, dev_t);
int					mbrinit(struct dkdevice *, dev_t, struct disklabel *, struct diskslices **);
int					gptinit(struct dkdevice *, dev_t, struct disklabel *, struct diskslices **);
#endif /* _KERNEL */
#endif /* _SYS_DISKSLICE_H_ */
