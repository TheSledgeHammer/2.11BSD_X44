/*-
 * Copyright (c) 1988, 1993
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

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char copyright[] =
"@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)fstat.c	8.3 (Berkeley) 5/2/95";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/mount.h>

#include <fs/msdosfs/denode.h>
#include <fs/msdosfs/bpb.h>
#define	_KERNEL
#include <fs/msdosfs/msdosfsmount.h>
#undef _KERNEL

#include <kvm.h>
#include <err.h>
#include "fstat.h"

int
msdosfs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct denode de;
	struct msdosfsmount mp;

	if (!KVM_READ(VTODE(vp), &de, sizeof(de))) {
		dprintf("can't read denode at %p for pid %d", VTODE(vp),
		    Pid);
		return (0);
	}
	if (!KVM_READ(de.de_pmp, &mp, sizeof(mp))) {
		dprintf("can't read mount struct at %p for pid %d", de.de_pmp,
		    Pid);
		return (0);
	}

	fsp->fsid = de.de_dev & 0xffff;
	fsp->fileid = 0; /* XXX see msdosfs_vptofh() for more info */
	fsp->size = de.de_FileSize;
	fsp->rdev = 0;	/* msdosfs doesn't support device files */
	fsp->mode = (0777 & mp.pm_mask) | getftype(vp->v_type);

	return (1);
}
