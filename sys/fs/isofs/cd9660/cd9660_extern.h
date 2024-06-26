/*-
 * Copyright (c) 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley
 * by Pace Willisson (pace@blitz.com).  The Rock Ridge Extension
 * Support code is derived from software contributed to Berkeley
 * by Atsushi Murai (amurai@spec.co.jp).
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
 *	@(#)iso.h	8.6 (Berkeley) 5/10/95
 */

#ifndef _ISOFS_CD9660_EXTERN_H_
#define _ISOFS_CD9660_EXTERN_H_

#include <sys/mount.h>

struct fid;
struct iso_directory_record;
struct mbuf;
struct nameidata;
struct netexport;
struct proc;
struct statfs;
struct ucred;
struct vfsconf;

/* CD-ROM Format type */
enum ISO_FTYPE  { ISO_FTYPE_DEFAULT, ISO_FTYPE_9660, ISO_FTYPE_RRIP, ISO_FTYPE_ECMA };

#ifndef	ISOFSMNT_ROOT
#define	ISOFSMNT_ROOT	0
#endif

struct iso_mnt {
	int im_flags;
	int im_joliet_level;

	struct mount *im_mountp;
	dev_t im_dev;
	struct vnode *im_devvp;

	int logical_block_size;
	int im_bshift;
	int im_bmask;

	int volume_space_size;
	struct netexport im_export;

	char root[ISODCL (157, 190)];
	int root_extent;
	int root_size;
	enum ISO_FTYPE  iso_ftype;

	int rr_skip;
	int rr_skip0;
};

#define VFSTOISOFS(mp)			((struct iso_mnt *)((mp)->mnt_data))

#define blkoff(imp, loc)		((loc) & (imp)->im_bmask)
#define lblktosize(imp, blk)	((blk) << (imp)->im_bshift)
#define lblkno(imp, loc)		((loc) >> (imp)->im_bshift)
#define blksize(imp, ip, lbn)	((imp)->logical_block_size)

__BEGIN_DECLS
int cd9660_mount(struct mount *, char *, caddr_t, struct nameidata *, struct proc *);
int cd9660_start(struct mount *, int, struct proc *);
int cd9660_unmount(struct mount *, int, struct proc *);
int cd9660_root(struct mount *, struct vnode **);
int cd9660_quotactl(struct mount *, int, uid_t, caddr_t, struct proc *);
int cd9660_statfs(struct mount *, struct statfs *, struct proc *);
int cd9660_sync(struct mount *, int, struct ucred *, struct proc *);
int cd9660_vget(struct mount *, ino_t, struct vnode **);
int cd9660_vget_internal(struct mount *, ino_t, struct vnode **, int, struct iso_directory_record *);
int cd9660_fhtovp(struct mount *, struct fid *, struct mbuf *, struct vnode **, int *, struct ucred **);
int cd9660_vptofh(struct vnode *, struct fid *);
int cd9660_init(struct vfsconf *);
int cd9660_sysctl(int *, u_int, void *, size_t *, void *, size_t, struct proc *);
int cd9660_mountroot(void);

int isochar(const u_char *, const u_char *, int, u_char *);
int isofncmp(const u_char *, int, const u_char *, int, int);
void isofntrans(u_char *, int, u_char *, u_short *, int, int, int, int);
ino_t isodirino(struct iso_directory_record *, struct iso_mnt *);
__END_DECLS

extern struct vnodeops cd9660_vnodeops;
extern struct vnodeops cd9660_specops;
#ifdef FIFO
extern struct vnodeops cd9660_fifoops;
#endif

#endif /* _ISOFS_CD9660_EXTERN_H_ */
