/* $NetBSD: lfsv1.c,v 1.16 2022/11/17 06:40:39 chs Exp $ */

#define	ufs_open		lfs_open
#define	ufs_close		lfs_close
#define	ufs_read		lfs_read
#define	ufs_write		lfs_write
#define	ufs_seek		lfs_seek
#define	ufs_stat		lfs_stat

#include "lib/libsa/ufs.c"
