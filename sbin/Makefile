#	$NetBSD: Makefile,v 1.134 2018/09/30 15:56:46 jmcneill Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

.include <bsd.own.mk>

SUBDIR=	disklabel fdisk fsck init ldconfig mknod mount swapctl

# support for various file systems
SUBDIR+= newfs dumpfs
SUBDIR+= newlfs dumplfs
SUBDIR+= newfs_msdos fsck_msdos
SUBDIR+= mount_cd9660
SUBDIR+= mount_fdesc
SUBDIR+= mount_lfs
SUBDIR+= mount_lofs
SUBDIR+= mount_msdos
SUBDIR+= mount_union

.include <bsd.subdir.mk>
