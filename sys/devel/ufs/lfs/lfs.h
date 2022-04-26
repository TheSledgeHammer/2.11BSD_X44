/*-
 * Copyright (c) 1991, 1993
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
 *
 *	@(#)lfs.h	8.9 (Berkeley) 5/8/95
 */

struct lfs {
	u_int32_t 				lfs_flag;				/* flags, see below */

	/*
	 * The on-disk dinode itself.
	 */
	union {
		struct ufs1_dinode 	*ffs1_din;
		struct ufs2_dinode 	*ffs2_din;
	} lfs_din;
};

static inline bool_t
I_IS_LFS1(const struct lfs *lfs)
{
	return ((lfs->lfs_flag & IN_UFS2) == 0);
}

static inline bool_t
I_IS_LFS2(const struct lfs *lfs)
{
	return ((lfs->lfs_flag & IN_UFS2) != 0);
}

#define	LFS_DIP(lfs, field)	(I_IS_LFS1(lfs) ?			\
		(lfs)->lfs_din.ffs1_din->di_##field :			\
		(lfs)->lfs_din.ffs2_din->di_##field)

#define	LFS_DIP_SET(lfs, field, val) do {				\
	if (I_IS_LFS1(lfs)) {								\
		(lfs)->lfs_din.ffs1_din->di_##field = (val);	\
	} else {											\
		(lfs)->lfs_din.ffs2_din->di_##field = (val);	\
	}													\
} while(0)
